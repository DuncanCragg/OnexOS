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

#include "onl/onl.h"

#define ERR_EXIT(msg) \
    do {                             \
        printf("%s\n", msg);     \
        fflush(stdout);              \
        exit(1);                     \
    } while (0)

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define VK_DESTROY(func, dev, obj) func(dev, obj, NULL), obj = NULL
#define VK_CHECK(r) do { VkResult res = (r); if (res != VK_SUCCESS){ printf("r=%d @ line %d\n", r, __LINE__); exit(1); } } while (0)

extern VkFormat surface_format;
extern VkDevice device;
extern VkPhysicalDevice gpu;
extern VkCommandBuffer initcmd;
extern VkQueue queue;
extern VkCommandPool command_pool;
extern VkSwapchainKHR swapchain;
extern VkExtent2D swapchain_extent;

extern bool prepared;

void ont_vk_restart();

void onx_prepare_swapchain_images(bool restart);
void onx_prepare_semaphores_and_fences(bool restart);
void onx_prepare_command_buffers(bool restart);
void onx_prepare_render_data(bool restart);
void onx_prepare_uniform_buffers(bool restart);
void onx_prepare_descriptor_layout(bool restart);
void onx_prepare_descriptor_pool(bool restart);
void onx_prepare_descriptor_set(bool restart);
void onx_prepare_render_pass(bool restart);
void onx_prepare_pipeline(bool restart);
void onx_prepare_framebuffers(bool restart);

void onx_init(bool restart);
void onx_render_frame();
void onx_iostate_changed();
void onx_finish();

#endif
