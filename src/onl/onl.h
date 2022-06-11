#ifndef ONL_ONL_H
#define ONL_ONL_H

#include <vulkan/vulkan.h>

typedef struct iostate {
  uint32_t view_width;
  uint32_t view_height;
  uint32_t swap_width;
  uint32_t swap_height;
  int16_t  rotation_angle;
  uint32_t mouse_x;
  uint32_t mouse_y;
  bool     left_pressed;
  bool     middle_pressed;
  bool     right_pressed;
  char     key;
} iostate;

extern iostate io;

static void set_io_rotation(int16_t a){

  io.rotation_angle = a;

  if(io.rotation_angle){  // currently only 0 or 90' allowed!
    io.view_width =io.swap_height;
    io.view_height=io.swap_width;
  }
  else{
    io.view_width =io.swap_width;
    io.view_height=io.swap_height;
  }
}

static void set_io_mouse(int32_t x, int32_t y){

  if(io.rotation_angle){ // == 90
    io.mouse_x = io.swap_height-y;
    io.mouse_y = x;
  }
  else{
    io.mouse_x = x;
    io.mouse_y = y;
  }
}

void onl_init();
void onl_create_window();
void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface);
void onl_run();
void onl_finish();

#endif
