
#include <stdbool.h>

#include "onl.h"

bool quit = false;

iostate io;

void set_io_rotation(int16_t a){

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

void set_io_mouse(int32_t x, int32_t y){

  if(io.rotation_angle){ // == 90
    io.mouse_x = io.swap_height-y;
    io.mouse_y = x;
  }
  else{
    io.mouse_x = x;
    io.mouse_y = y;
  }
}

