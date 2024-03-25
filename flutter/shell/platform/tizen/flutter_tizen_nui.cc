// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include <dali-toolkit/public-api/controls/image-view/image-view.h>
#include <dali/devel-api/adaptor-framework/native-image-source-queue.h>

#include <memory>

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_view_nui.h"

namespace {

// Returns the engine corresponding to the given opaque API handle.
flutter::FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

FlutterDesktopViewRef HandleForView(flutter::FlutterTizenView* view) {
  return reinterpret_cast<FlutterDesktopViewRef>(view);
}

}  // namespace

FlutterDesktopViewRef FlutterDesktopViewCreateFromImageView(
    const FlutterDesktopViewProperties& view_properties,
    FlutterDesktopEngineRef engine,
    void* image_view,
    void* native_image_queue,
    int32_t default_window_id) {
  std::unique_ptr<flutter::TizenViewBase> tizen_view =
      std::make_unique<flutter::TizenViewNui>(
          view_properties.width, view_properties.height,
          reinterpret_cast<Dali::Toolkit::ImageView*>(image_view),
          reinterpret_cast<Dali::NativeImageSourceQueue*>(native_image_queue),
          default_window_id);

  auto view = std::make_unique<flutter::FlutterTizenView>(
      flutter::kImplicitViewId, std::move(tizen_view));

  // Take ownership of the engine, starting it if necessary.
  view->SetEngine(
      std::unique_ptr<flutter::FlutterTizenEngine>(EngineFromHandle(engine)));
  view->CreateRenderSurface(FlutterDesktopRendererType::kEGL);
  if (!view->engine()->IsRunning()) {
    if (!view->engine()->RunEngine()) {
      return nullptr;
    }
  }

  view->SendInitialGeometry();

  return HandleForView(view.release());
}
