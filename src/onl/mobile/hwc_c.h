#ifndef HWC_C_H
#define HWC_C_H

#include <android/native_window.h>
#include <EGL/egl.h>

ANativeWindow* hwc_create_hwcomposer_window();
uint32_t       hwc_get_swap_width();
uint32_t       hwc_get_swap_height();
void           hwc_display_on();
void           hwc_display_brightness(uint8_t brightness);
void           hwc_display_off();
void           hwc_destroy_hwcomposer_window();

#endif


