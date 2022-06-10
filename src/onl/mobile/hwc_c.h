#ifndef LIBHYBRIS_HWC_C_H
#define LIBHYBRIS_HWC_C_H

#include <android/native_window.h>
#include <EGL/egl.h>

ANativeWindow* hwc_create_hwcomposer_window();
void           hwc_display_on();
void           hwc_display_off();
void           hwc_destroy_hwcomposer_window();

#endif


