// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_H_
#define EMBEDDER_TIZEN_RENDERER_H_

#include <cstdint>
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

class TizenRenderer {
 public:
  TizenRenderer();

  virtual ~TizenRenderer();
  bool IsValid() { return is_valid_; }

  virtual void ResizeSurface(int32_t width, int32_t height) = 0;

  virtual FlutterRendererConfig GetRendererConfig() = 0;
  virtual std::unique_ptr<ExternalTexture> CreateExternalTexture(
      const FlutterDesktopTextureInfo* texture_info) = 0;

 protected:
  bool CreateSurface(TizenViewBase* view);
  virtual bool CreateSurface(void* render_target,
                             void* render_target_display,
                             int32_t width,
                             int32_t height) = 0;
  virtual void DestroySurface() = 0;
  bool is_valid_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_H_
