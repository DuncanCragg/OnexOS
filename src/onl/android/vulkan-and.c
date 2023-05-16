
#include <stdint.h>
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

#include <android/native_window.h>
#include <android/native_activity.h>
#include <android/asset_manager.h>
#include <android_native_app_glue.h>
#include <sys/system_properties.h>

#include <onex-kernel/log.h>

#include "ont/unix/vulkan/vk.h"
#include "onl/onl.h"

ANativeWindow* window=0;

struct android_app* android_app_state;

void onl_init(){
  log_write("=========== onl_init: prepare(false)\n");
}

void onl_finish() {
  log_write("=========== onl_finish: finish(false)\n");
}

void onl_create_window(){

  // window not created here but when android event comes in

  io.swap_width =1080;
  io.swap_height=1920;

  set_io_rotation(0);
}

void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface){

  VkAndroidSurfaceCreateInfoKHR android_surface_ci = {
    .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
    .flags = 0,
    .window = window,
  };
  VkResult err = vkCreateAndroidSurfaceKHR(inst, &android_surface_ci, 0, surface);
  assert(!err);
}

bool focused = true;

void handle_app_command(struct android_app* app, int32_t cmd) {

  switch (cmd) {
    case APP_CMD_SAVE_STATE:
      log_write("APP_CMD_SAVE_STATE");
      break;
    case APP_CMD_INIT_WINDOW:
      if(android_app_state->window) {
          window=android_app_state->window;
      }
      else log_write("No window assigned!");
      break;
    case APP_CMD_LOST_FOCUS:
      log_write("APP_CMD_LOST_FOCUS");
      focused = false;
      break;
    case APP_CMD_GAINED_FOCUS:
      log_write("APP_CMD_GAINED_FOCUS");
      focused = true;
      break;
    case APP_CMD_TERM_WINDOW:
      log_write("APP_CMD_TERM_WINDOW");
      window=0;
      break;
    case APP_CMD_DESTROY:
      log_write("APP_CMD_DESTROY");
      break;
  }
}

int32_t handle_app_input(struct android_app* app, AInputEvent* event) {

  if(AInputEvent_getType(event)==AINPUT_EVENT_TYPE_MOTION) {

    int32_t eventSource = AInputEvent_getSource(event);

    switch (eventSource) {
      case AINPUT_SOURCE_JOYSTICK: {
        break;
      }
      case AINPUT_SOURCE_TOUCHSCREEN: {
        int32_t action = AMotionEvent_getAction(event);

        switch (action) {
          case AMOTION_EVENT_ACTION_UP: {
            return 1;
          }
          case AMOTION_EVENT_ACTION_DOWN: {
            break;
          }
          case AMOTION_EVENT_ACTION_MOVE: {
            int32_t eventX = AMotionEvent_getX(event, 0);
            int32_t eventY = AMotionEvent_getY(event, 0);
            break;
          }
          default:
            return 1;
            break;
        }
      }
      return 1;
    }
  }

  if(AInputEvent_getType(event)==AINPUT_EVENT_TYPE_KEY){

    int32_t keyCode = AKeyEvent_getKeyCode((const AInputEvent*)event);
    int32_t action = AKeyEvent_getAction((const AInputEvent*)event);

    if (action == AKEY_EVENT_ACTION_UP){
      if(keyCode == AKEYCODE_BACK){
        return 1;
      }
      return 0;
    }

    switch (keyCode) {
    case AKEYCODE_BACK:
      break;
    case AKEYCODE_BUTTON_A:
      break;
    case AKEYCODE_BUTTON_B:
      break;
    case AKEYCODE_BUTTON_X:
      break;
    case AKEYCODE_BUTTON_Y:
      break;
    case AKEYCODE_BUTTON_L1:
      break;
    case AKEYCODE_BUTTON_R1:
      break;
    case AKEYCODE_BUTTON_START:
      break;
    };
  }

  return 0;
}

static void event_loop(){

  while(true){

    int ident;
    int events;
    struct android_poll_source* source;
    bool destroy_requested=false;
    while((ident=ALooper_pollAll(focused? 0: 200, 0, &events, (void**)&source)) >= 0){

      if(source) source->process(android_app_state, source);

      if(android_app_state->destroyRequested) {
        log_write("destroy requested");
        destroy_requested=true;
        break;
      }
    }

    ont_vk_loop(!!window); // && focused? i.e. finish(true) - like temp restart

    if(destroy_requested) return;

/*
    if(!focused){
      if(ident==ALOOPER_POLL_TIMEOUT) continue;
    }
*/
  }
}

void android_main(struct android_app* state) {

  log_write("----------------------\nStarting onx (OnexOS)\n-----------------------\n");

  android_app_state = state;
  android_app_state->onAppCmd     = handle_app_command;
  android_app_state->onInputEvent = handle_app_input;

  event_loop();
}

void onl_exit(int n){
  log_write("--------\nEnding onx (OnexOS)\n---------\n");
  ANativeActivity_finish(android_app_state->activity);
}


