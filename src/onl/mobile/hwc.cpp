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

hwc2_compat_device_t*  hwcDevice;
hwc2_compat_display_t* hwcDisplay;

HWComposer2::HWComposer2(unsigned int width, unsigned int height,
                         unsigned int format,
                         hwc2_compat_display_t* display,
                         hwc2_compat_layer_t*   layer    ):

             HWComposerNativeWindow(width, height, format) {

  this->hwcDisplay = display;
  this->layer      = layer;
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

  int fenceFd = hwc2_compat_out_fences_get_fence(fences, layer);
  if (fenceFd != -1) setFenceBufferFd(buffer, fenceFd);

  hwc2_compat_out_fences_destroy(fences);

  if (lastPresentFence != -1) {
    sync_wait(lastPresentFence, -1);
    close(lastPresentFence);
  }
  lastPresentFence = presentFence;
}

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

HWComposer2 *create_hwcomposer_window()
{
  int composerSequenceId = 0;

  hwcDevice = hwc2_compat_device_new(false);
  assert(hwcDevice);

  hwc2_compat_device_register_callback(hwcDevice, &eventListener, composerSequenceId);

  for (int i = 0; i < 5 * 1000; ++i) {
    if ((hwcDisplay = hwc2_compat_device_get_display_by_id(hwcDevice, 0))){
      break;
    }
    usleep(1000);
  }

  assert(hwcDisplay);

  HWC2DisplayConfig* config = hwc2_compat_display_get_active_config(hwcDisplay);

  printf("HWC: width: %i height: %i\n", config->width, config->height);

  hwc2_compat_layer_t* layer = hwc2_compat_display_create_layer(hwcDisplay);

  hwc2_compat_layer_set_composition_type(layer, HWC2_COMPOSITION_CLIENT);
  hwc2_compat_layer_set_blend_mode(layer, HWC2_BLEND_MODE_NONE);
  hwc2_compat_layer_set_source_crop(layer, 0.0f, 0.0f,
                                    config->width, config->height);
  hwc2_compat_layer_set_display_frame(layer, 0, 0,
                                      config->width, config->height);
  hwc2_compat_layer_set_visible_region(layer, 0, 0,
                                       config->width, config->height);

  HWComposer2 *win = new HWComposer2(config->width, config->height,
                                     HAL_PIXEL_FORMAT_RGBA_8888,
                                     hwcDisplay, layer);
  return win;
}

extern "C" {

ANativeWindow* hwc_create_hwcomposer_window(){
  return create_hwcomposer_window();
}

void hwc_display_on(){
  hwc2_compat_display_set_power_mode(hwcDisplay, HWC2_POWER_MODE_ON);
}

void hwc_display_off(){
  hwc2_compat_display_set_power_mode(hwcDisplay, HWC2_POWER_MODE_OFF);
}

}


