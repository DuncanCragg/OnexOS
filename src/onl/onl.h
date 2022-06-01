#ifndef ONL_ONL_H
#define ONL_ONL_H

#include <vulkan/vulkan.h>

void onl_init();
void onl_create_window();
void onl_create_surface(VkInstance inst, VkSurfaceKHR* surface);
void onl_run();
void onl_finish();

#endif
