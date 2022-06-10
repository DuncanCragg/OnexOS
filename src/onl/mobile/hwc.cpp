/*
 * Copyright (C) 2020 Matti Lehtimäki <matti.lehtimaki@gmail.com>
 * Copyright (C) 2018 TheKit <nekit1000@gmail.com>
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <android-config.h>

#include <stdio.h>
#include <stdlib.h>

#include <cutils/log.h>
#include <sync/sync.h>

#include "hwc.h"
#include "logging.h"
extern "C" {
#include "hwc_c.h"
}

static HWComposer2*          the_hwc2=0;
static HWComposer2*          the_other_hwc2=0;
static HWComposer2*          the_third_hwc2=0;
static hwc2_compat_device_t* hwcDevice;

// --------------------------------------------------------- {

HWComposer2::HWComposer2(unsigned int width, unsigned int height,
                         unsigned int format,
                         hwc2_compat_display_t* display,
                         hwc2_compat_layer_t*   layer    ):

             HWComposerNativeWindow(width, height, format) {

  this->width      = width;
  this->height     = height;
  this->hwcDisplay = display;
  this->hwcLayer   = layer;
}

void HWComposer2::present(HWComposerNativeWindowBuffer *buffer)
{
  uint32_t numTypes = 0;
  uint32_t numRequests = 0;

  hwc2_error_t error = hwc2_compat_display_validate(hwcDisplay,
                                                    &numTypes,
                                                    &numRequests);

  if (error != HWC2_ERROR_NONE && error != HWC2_ERROR_HAS_CHANGES) {
    HYBRIS_ERROR("prepare: validate failed for display %lu: %s (%d)",
                  displayId, to_string(static_cast<HWC2::Error>(error)).c_str(), error);
    return;
  }

  if (numTypes || numRequests) {
    HYBRIS_ERROR("prepare: validate required changes for display %lu: %s (%d)",
                  displayId, to_string(static_cast<HWC2::Error>(error)).c_str(),
      error);
    return;
  }

  error = hwc2_compat_display_accept_changes(hwcDisplay);
  if (error != HWC2_ERROR_NONE) {
    HYBRIS_ERROR("prepare: acceptChanges failed: %s",
                  to_string(static_cast<HWC2::Error>(error)).c_str());
    return;
  }

  hwc2_compat_display_set_client_target(hwcDisplay, /* slot */0, buffer,
                                        getFenceBufferFd(buffer),
                                        HAL_DATASPACE_UNKNOWN);

  int presentFence;
  hwc2_compat_display_present(hwcDisplay, &presentFence);

  if (error != HWC2_ERROR_NONE) {
    HYBRIS_ERROR("presentAndGetReleaseFences: failed for display %lu: %s (%d)",
                  displayId, to_string(static_cast<HWC2::Error>(error)).c_str(), error);
    return;
  }

  hwc2_compat_out_fences_t* fences;
  error = hwc2_compat_display_get_release_fences(hwcDisplay, &fences);

  if (error != HWC2_ERROR_NONE) {
    HYBRIS_ERROR("presentAndGetReleaseFences: Failed to get release fences for display %lu: %s (%d)",
                  displayId, to_string(static_cast<HWC2::Error>(error)).c_str(), error);
    return;
  }

  int fenceFd = hwc2_compat_out_fences_get_fence(fences, hwcLayer);
  if (fenceFd != -1) setFenceBufferFd(buffer, fenceFd);

  hwc2_compat_out_fences_destroy(fences);

  if (lastPresentFence != -1) {
    sync_wait(lastPresentFence, -1);
    close(lastPresentFence);
  }
  lastPresentFence = presentFence;
}

// } --------------------------------------------------------- {

void onVsyncReceived(HWC2EventListener* listener, int32_t sequenceId,
                     hwc2_display_t display, int64_t timestamp)
{
  printf("onVsyncReceived %ld\n", timestamp);
}

void onHotplugReceived(HWC2EventListener* listener, int32_t sequenceId,
                       hwc2_display_t display, bool connected,
                       bool primaryDisplay)
{
  HYBRIS_INFO("onHotplugReceived(%d, %" PRIu64 ", %s, %s)",
               sequenceId, display,
               connected ? "connected" : "disconnected",
               primaryDisplay ? "primary" : "external");

  hwc2_compat_device_on_hotplug(hwcDevice, display, connected);
}

void onRefreshReceived(HWC2EventListener* listener,
                       int32_t sequenceId, hwc2_display_t display)
{
}

HWC2EventListener eventListener = {
    &onVsyncReceived,
    &onHotplugReceived,
    &onRefreshReceived
};

// } --------------------------------------------------------- {

HWComposer2 *create_hwcomposer_window() {

  int composerSequenceId = 0;

  hwcDevice = hwc2_compat_device_new(false);
  assert(hwcDevice);

  hwc2_compat_device_register_callback(hwcDevice, &eventListener, composerSequenceId);

  hwc2_compat_display_t* display;
  for (int i = 0; i < 5 * 1000; ++i) {
    if ((display = hwc2_compat_device_get_display_by_id(hwcDevice, 0))){
      break;
    }
    usleep(1000);
  }
  assert(display);

  HWC2DisplayConfig* config = hwc2_compat_display_get_active_config(display);

  printf("HWC: swap_width: %i swap_height: %i\n", config->width, config->height);

  hwc2_compat_layer_t* layer = hwc2_compat_display_create_layer(display);

  hwc2_compat_layer_set_composition_type(layer, HWC2_COMPOSITION_CLIENT);
  hwc2_compat_layer_set_blend_mode(layer, HWC2_BLEND_MODE_NONE);
  hwc2_compat_layer_set_source_crop(layer, 0.0f, 0.0f,
                                    config->width, config->height);
  hwc2_compat_layer_set_display_frame(layer, 0, 0,
                                      config->width, config->height);
  hwc2_compat_layer_set_visible_region(layer, 0, 0,
                                       config->width, config->height);

  the_hwc2 = new HWComposer2(config->width, config->height,
                             HAL_PIXEL_FORMAT_RGBA_8888,
                             display, layer);
  the_other_hwc2 = the_hwc2;
  return the_hwc2;
}

static hwc2_compat_display_t* get_display(){
  return the_other_hwc2->hwcDisplay;
}

static hwc2_compat_layer_t* get_layer(){
  return the_other_hwc2->hwcLayer;
}

static uint32_t get_swap_width(){
  return the_other_hwc2->width;
}

static uint32_t get_swap_height(){
  return the_other_hwc2->height;
}

// } ---------------------------------------------------------

extern "C" {

ANativeWindow* hwc_create_hwcomposer_window(){
  return create_hwcomposer_window();
}

uint32_t hwc_get_swap_width(){
  return get_swap_width();
}

uint32_t hwc_get_swap_height(){
  return get_swap_height();
}

void hwc_display_on(){
  hwc2_compat_display_set_power_mode(get_display(), HWC2_POWER_MODE_ON);
}

void hwc_display_brightness(uint8_t brightness){
  char cmdbuf[128];
  snprintf(cmdbuf, 128, "echo %d > /sys/class/leds/lcd-backlight/brightness", brightness);
  system(cmdbuf);
}

void hwc_display_off(){
  the_third_hwc2=the_hwc2;
  hwc2_compat_display_set_power_mode(get_display(), HWC2_POWER_MODE_OFF);
}

void hwc_destroy_hwcomposer_window(){
  hwc2_compat_display_destroy_layer(get_display(), get_layer());
}

}


