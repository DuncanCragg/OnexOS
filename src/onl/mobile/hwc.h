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
#ifndef HWC_H
#define HWC_H

#include <EGL/egl.h>
#include <hwcomposer_window.h>
#include <hybris/hwc2/hwc2_compatibility_layer.h>

class HWComposer2 : public HWComposerNativeWindow
{
public:
  HWComposer2(unsigned int width, unsigned int height,
              unsigned int format,
              hwc2_compat_display_t* display,
              hwc2_compat_layer_t*   layer);

  unsigned int width;
  unsigned int height;
  hwc2_compat_display_t* hwcDisplay;
  hwc2_compat_layer_t*   hwcLayer;
protected:
  void present(HWComposerNativeWindowBuffer *buffer);
private:
  int lastPresentFence = -1;
};

HWComposer2 *create_hwcomposer_window();

#endif
