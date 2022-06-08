#ifndef ONT_VULKAN_VULKAN_UP
#define ONT_VULKAN_VULKAN_UP

void ont_vk_loop();
void ont_vk_handle_event(char key_pressed, char key_released,
                         uint32_t mouse_x, uint32_t mouse_y,
                         bool left_pressed, bool middle_pressed, bool right_pressed,
                         bool left_released, bool middle_released, bool right_released,
                         uint32_t w, uint32_t h);
#endif
