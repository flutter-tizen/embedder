// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_NUI_H_
#define EMBEDDER_TIZEN_RENDERER_NUI_H_

#include <cstdint>
#include <memory>
#include "flutter/shell/platform/tizen/tizen_renderer_egl.h"
#include "flutter/shell/platform/tizen/tizen_renderer_gl.h"
#include "flutter/shell/platform/tizen/tizen_view_nui.h"

namespace flutter {

class TizenRendererNuiGL : public TizenRendererEgl {
 public:
  explicit TizenRendererNuiGL(TizenViewNui* view_nui, bool enable_impeller);

  virtual ~TizenRendererNuiGL();

  bool OnPresent() override;

 private:
  TizenViewNui* view_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_NUI_H_
