// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_view_elementary.h"

namespace {

// Returns the engine corresponding to the given opaque API handle.
flutter::FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

FlutterDesktopViewRef HandleForView(flutter::FlutterTizenView* view) {
  return reinterpret_cast<FlutterDesktopViewRef>(view);
}

}  // namespace

FlutterDesktopViewRef FlutterDesktopViewCreateFromElmParent(
    const FlutterDesktopViewProperties& view_properties,
    FlutterDesktopEngineRef engine,
    void* parent) {
  std::unique_ptr<flutter::TizenViewBase> tizen_view =
      std::make_unique<flutter::TizenViewElementary>(
          view_properties.width, view_properties.height,
          static_cast<Evas_Object*>(parent));

  auto view =
      std::make_unique<flutter::FlutterTizenView>(std::move(tizen_view));

  // Starting it if necessary.
  view->SetEngine(EngineFromHandle(engine));
  view->CreateRenderSurface(FlutterDesktopRendererType::kEvasGL);
  if (!view->engine()->IsRunning()) {
    if (!view->engine()->RunOrSpawnEngine()) {
      return nullptr;
    }
  }

  view->SendInitialGeometry();

  return HandleForView(view.release());
}
