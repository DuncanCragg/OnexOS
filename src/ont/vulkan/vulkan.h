#ifndef ONT_VULKAN_VULKAN
#define ONT_VULKAN_VULKAN

/* Platform-independent Vulkan common code */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>

#include <vulkan/vulkan.h>

#define ERR_EXIT(msg) \
    do {                             \
        printf("%s\n", msg);     \
        fflush(stdout);              \
        exit(1);                     \
    } while (0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define VK_DESTROY(func, dev, obj) func(dev, obj, NULL), obj = NULL
#define VK_CHECK(r) do { VkResult res = (r); if (res != VK_SUCCESS){ printf("r=%d @ line %d\n", r, __LINE__); exit(1); } } while (0)

extern int width, height;

extern uint32_t image_count;
extern uint32_t image_index;

extern VkFormat surface_format;
extern VkDevice device;
extern VkPhysicalDevice gpu;
extern VkCommandBuffer initcmd;
extern VkQueue queue;
extern VkCommandPool command_pool;
extern VkSwapchainKHR swapchain;
extern VkExtent2D swapchain_extent;
extern VkRenderPass render_pass;

typedef struct iostate {
  uint32_t mouse_x;
  uint32_t mouse_y;
  char     key;
  bool     left_pressed;
  bool     middle_pressed;
  bool     right_pressed;
} iostate;

iostate ont_vk_get_iostate();

void ont_vk_restart();

void onx_prepare_swapchain_images();
void onx_prepare_semaphores_and_fences();
void onx_prepare_command_buffers();
void onx_prepare_render_data();
void onx_prepare_uniform_buffers();
void onx_prepare_descriptor_layout();
void onx_prepare_descriptor_pool();
void onx_prepare_descriptor_set();
void onx_prepare_render_pass();
void onx_prepare_pipeline();
void onx_prepare_framebuffers();
void onx_render_pass();
void onx_render_frame();
void onx_destroy_objects();
void onx_handle_event(iostate io);

#endif
