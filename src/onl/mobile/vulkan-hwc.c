
/* Wiring between Vulkan and HWC */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include <EGL/egl.h>
#include <android/native_window.h>

#include "onl/onl.h"

#include "hwc_c.h"

ANativeWindow* window;

void onl_init(){
}

void onl_create_window(){

  window = create_hwcomposer_window_c();

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

bool quit = false;

extern void ont_vk_loop();

void onl_run(){
  while (!quit) {
    ont_vk_loop();
  }
}

void onl_finish(){
  printf("onl_finish\n");
}



