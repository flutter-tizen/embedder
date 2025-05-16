// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <Ecore.h>
#include <system_info.h>

#include "flutter/shell/platform/tizen/flutter_tizen_display_monitor.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/system_utils.h"

namespace flutter {

FlutterTizenDisplayMonitor::FlutterTizenDisplayMonitor(
    FlutterTizenEngine* engine)
    : engine_(engine) {}

FlutterTizenDisplayMonitor::~FlutterTizenDisplayMonitor() {}

void FlutterTizenDisplayMonitor::UpdateDisplays() {
  // TODO: Currently Tizen only supports one display device.
  // So, if Tizen supports multiple displays in the future, please implement to
  // get information about multiple displays.

  FlutterEngineDisplay display = {};
  display.struct_size = sizeof(FlutterEngineDisplay);
  display.display_id = 0;
  display.single_display = true;

  double fps = ecore_animator_frametime_get();
  if (fps <= 0.0) {
    display.refresh_rate = 0.0;
  } else {
    display.refresh_rate = 1 / fps;
  }

  int32_t width = 0, height = 0, dpi = 0;
  FlutterTizenView* view = engine_->view();
  if (view) {
    TizenGeometry geometry = view->tizen_view()->GetGeometry();
    width = geometry.width;
    height = geometry.height;
    dpi = view->tizen_view()->GetDpi();
  } else {
    system_info_get_platform_int("http://tizen.org/feature/screen.width",
                                 &width);
    system_info_get_platform_int("http://tizen.org/feature/screen.height",
                                 &height);
    system_info_get_platform_int("http://tizen.org/feature/screen.dpi", &dpi);
  }

  display.width = width;
  display.height = height;
  display.device_pixel_ratio = ComputePixelRatio(dpi);

  std::vector<FlutterEngineDisplay> displays = {display};
  engine_->UpdateDisplay(displays);
}

}  // namespace flutter
