#ifndef ONL_H
#define ONL_H

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

void set_io_rotation(int16_t a);
void set_io_mouse(int32_t x, int32_t y);

extern bool quit;

void onl_init();
void onl_create_window();
void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface);
void onl_finish();
void onl_exit(int n);

#endif
