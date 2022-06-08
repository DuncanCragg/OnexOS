
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

extern void ont_vk_loop();
extern void ont_vk_handle_event(char key_pressed, char key_released,
                                int32_t mouse_x, int32_t mouse_y,
                                bool left_pressed, bool middle_pressed, bool right_pressed,
                                bool left_released, bool middle_released, bool right_released,
                                int w, int h);

#include "onl/onl.h"

#include "hwc_c.h"

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
    }
    libinput_event_destroy(event);
    libinput_dispatch(libin);
  }
}

void onl_create_window(){

  window = hwc_create_hwcomposer_window();

  hwc_display_on();

  EGLDisplay display = eglGetDisplay(NULL);

  int r = eglInitialize(display, 0, 0);

  if(!window || r!=EGL_TRUE || eglGetError() != EGL_SUCCESS){
    printf("eglInitialize didn't work\n"); exit(-1);
  }
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

static void handle_libinput_event(struct libinput_event* event) {

  switch (libinput_event_get_type(event)) {

    case LIBINPUT_EVENT_TOUCH_DOWN: {

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = (int32_t)libinput_event_touch_get_slot(t);
      int32_t x = (int32_t)libinput_event_touch_get_x(t);
      int32_t y = (int32_t)libinput_event_touch_get_y(t);

      touch_positions[s][0]=x;
      touch_positions[s][1]=y;

      if(s==0) ont_vk_handle_event(0,0, x,y, false,false,false, false,false,false, 0,0);
      if(s==1) ont_vk_handle_event(0,0, 0,0, true,false,false, false,false,false, 0,0);

      break;
    }
    case LIBINPUT_EVENT_TOUCH_MOTION: {

      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = (int32_t)libinput_event_touch_get_slot(t);
      int32_t x = (int32_t)libinput_event_touch_get_x(t);
      int32_t y = (int32_t)libinput_event_touch_get_y(t);

      if(touch_positions[s][0] != x ||
         touch_positions[s][1] != y   ){

        touch_positions[s][0]=x;
        touch_positions[s][1]=y;

        if(s==0) ont_vk_handle_event(0,0, x,y, false,false,false, false,false,false, 0,0);
      }
      break;
    }
    case LIBINPUT_EVENT_TOUCH_UP: {
      struct libinput_event_touch *t = libinput_event_get_touch_event(event);

      int32_t s = libinput_event_touch_get_slot(t);

      if(s==1) ont_vk_handle_event(0,0, 0,0, false,false,false, true,false,false, 0,0);

      break;
    }
  }
}

void onl_run(){

  struct pollfd fds;
  fds.fd = libinput_get_fd(libin);
  fds.events = POLLIN;
  fds.revents = 0;

  while(!quit && poll(&fds, 1, -1) > -1){

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

  hwc_display_off();
}



