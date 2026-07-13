// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_renderer.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

TizenRenderer::TizenRenderer() {}

bool TizenRenderer::CreateSurface(TizenViewBase* view) {
  TizenGeometry geometry = view->GetGeometry();
  auto* window = dynamic_cast<TizenWindow*>(view);
  return CreateSurface(window->GetRenderTarget(),
                       window->GetRenderTargetDisplay(), geometry.width,
                       geometry.height);
}

TizenRenderer::~TizenRenderer() = default;

}  // namespace flutter
