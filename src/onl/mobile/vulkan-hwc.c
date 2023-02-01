
/* Wiring between Vulkan and HWC */

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

#include <EGL/egl.h>
#include <android/native_window.h>

#include <libudev.h>
#include <libinput.h>
#include <libevdev/libevdev.h>

#include "hwc_c.h"

#include "ont/unix/vulkan/vulkan_up.h"
#include "onl/onl.h"

iostate io;

ANativeWindow* window;

bool quit = false;

static void sighandler(int signal, siginfo_t *siginfo, void *userdata) {
  printf("\nEnd\n");
  quit = true;
}

static int open_restricted(const char *path, int flags, void *user_data) {
  int fd = open(path, flags);
  return fd < 0 ? -errno : fd;
}

static void close_restricted(int fd, void *user_data) {
  close(fd);
}

static const struct libinput_interface interface = {
  .open_restricted = open_restricted,
  .close_restricted = close_restricted,
};

struct udev* udev;
struct libinput* libin;

void onl_init(){
}

static void init_libinput(){

  struct sigaction act;
  memset(&act, 0, sizeof(act));
  act.sa_sigaction = sighandler;
  act.sa_flags = SA_SIGINFO;
  sigaction(SIGINT, &act, 0);

  udev = udev_new();
  libin = libinput_udev_create_context(&interface, 0, udev);
  libinput_udev_assign_seat(libin, "seat0");

  struct libinput_event* event;

  libinput_dispatch(libin);

  while((event = libinput_get_event(libin))){
    switch (libinput_event_get_type(event)) {

      case LIBINPUT_EVENT_DEVICE_ADDED: {

        struct libinput_device* device  = libinput_event_get_device(event);
        struct udev_device*     udevice = libinput_device_get_udev_device(device);

        if(libinput_device_has_capability(device, LIBINPUT_DEVICE_CAP_TOUCH)){

          const char* devpath    = udev_device_get_devnode(udevice);
          const char* touch_prop = udev_device_get_property_value(udevice, "ID_INPUT_TOUCHSCREEN");

          printf("found touchscreen at %s touchscreen prop %s\n", devpath, touch_prop? touch_prop: "none");
        }
        break;
      }
      default: {
        printf("event that isn't device added!\n");
        break;
      }
    }
    libinput_event_destroy(event);
    libinput_dispatch(libin);
  }
}

static EGLDisplay display;

void onl_create_window(){

  window = hwc_create_hwcomposer_window();

  io.swap_width =hwc_get_swap_width();
  io.swap_height=hwc_get_swap_height();

  set_io_rotation(90);

  display = eglGetDisplay(NULL);

  int r = eglInitialize(display, 0, 0);

  if(!window || r!=EGL_TRUE || eglGetError() != EGL_SUCCESS){
    printf("eglInitialize didn't work\n"); exit(-1);
  }

  hwc_display_on();
  hwc_display_brightness(255);

  init_libinput();
}

void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface){

  VkAndroidSurfaceCreateInfoKHR android_surface_ci = {
    .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
    .flags = 0,
    .window = (ANativeWindow*)window,
  };
  VkResult err = vkCreateAndroidSurfaceKHR(inst, &android_surface_ci, 0, surface);
  assert(!err);
}

static int32_t touch_positions[8][2];

static uint32_t screen_on = 2;

static void handle_libinput_event(struct libinput_event* event) {

  switch (libinput_event_get_type(event)) {

    case LIBINPUT_EVENT_TOUCH_DOWN: {

      if(!screen_on) break;
      if(screen_on == 1){
        screen_on = 2;
        hwc_display_brightness(255);
      }

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = (int32_t)libinput_event_touch_get_slot(t);
      int32_t x = (int32_t)libinput_event_touch_get_x(t);
      int32_t y = (int32_t)libinput_event_touch_get_y(t);

      touch_positions[s][0]=x;
      touch_positions[s][1]=y;

      set_io_mouse(x, y);

      io.left_pressed=true;

      ont_vk_iostate_changed();

      break;
    }
    case LIBINPUT_EVENT_TOUCH_MOTION: {

      if(!screen_on) break;

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = (int32_t)libinput_event_touch_get_slot(t);
      int32_t x = (int32_t)libinput_event_touch_get_x(t);
      int32_t y = (int32_t)libinput_event_touch_get_y(t);

      if(touch_positions[s][0] == x &&
         touch_positions[s][1] == y   ) break;

      touch_positions[s][0]=x;
      touch_positions[s][1]=y;

      set_io_mouse(x, y);

      ont_vk_iostate_changed();

      break;
    }
    case LIBINPUT_EVENT_TOUCH_UP: {

      if(!screen_on) break;

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = libinput_event_touch_get_slot(t);

      io.left_pressed=false;

      ont_vk_iostate_changed();

      break;
    }
    case LIBINPUT_EVENT_KEYBOARD_KEY: {

      struct libinput_event_keyboard* k       = libinput_event_get_keyboard_event(event);
      uint32_t                        key     = libinput_event_keyboard_get_key(k);
      enum libinput_key_state         state   = libinput_event_keyboard_get_key_state(k);
      const char*                     keyname = libevdev_event_code_get_name(EV_KEY, key);

      if(key == KEY_POWER && state == LIBINPUT_KEY_STATE_PRESSED){
        if(!screen_on){
          hwc_display_on(); // if not already on, crashes libinput
          screen_on = 2;
          hwc_display_brightness(255);
        }
        else{
          screen_on = 0;
          hwc_display_brightness(0);

          if(io.left_pressed){
            io.left_pressed=false;
            ont_vk_iostate_changed();
          }
        }
      }
      else
      if(key == KEY_HOME && state == LIBINPUT_KEY_STATE_PRESSED){
        screen_on = 1;
        hwc_display_brightness(32);
      }
      else
      if(key == KEY_VOLUMEUP && state == LIBINPUT_KEY_STATE_PRESSED){
        screen_on = 0;
        hwc_display_off(); // for testing

        if(io.left_pressed){
          io.left_pressed=false;
          ont_vk_iostate_changed();
        }
      }
      else
      if(key == KEY_VOLUMEDOWN && state == LIBINPUT_KEY_STATE_PRESSED){
        set_io_rotation(io.rotation_angle? 0: 90);
        ont_vk_iostate_changed();
      }
      else{
        printf("%s (%d) %s\n", keyname, key, state == LIBINPUT_KEY_STATE_PRESSED? "pressed": "released");
      }
      break;
    }
    case LIBINPUT_EVENT_TOUCH_FRAME: {
      break;
    }
    default: {
      uint32_t e = libinput_event_get_type(event);
      printf("other event: %d\n", e);
    }
  }
}

void onl_run(){
/*
  struct pollfd fds;
  fds.fd = libinput_get_fd(libin);
  fds.events = POLLIN;
  fds.revents = 0;
*/
  while(!quit/* && poll(&fds, 1, -1) > -1*/){

    struct libinput_event* event;

    libinput_dispatch(libin);

    while((event = libinput_get_event(libin))){

      handle_libinput_event(event);

      libinput_event_destroy(event);
      libinput_dispatch(libin);
    }
    ont_vk_loop();
  }
}

void onl_finish(){

  libinput_unref(libin);
  udev_unref(udev);

  eglTerminate(display);

  hwc_display_brightness(0);

  hwc_destroy_hwcomposer_window();
}



