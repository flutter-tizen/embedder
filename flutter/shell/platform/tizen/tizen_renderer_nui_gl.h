// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_NUI_H_
#define EMBEDDER_TIZEN_RENDERER_NUI_H_

#include <cstdint>
#include <memory>
#include "flutter/shell/platform/tizen/tizen_renderer_gl.h"
#include "flutter/shell/platform/tizen/tizen_view_nui.h"

namespace flutter {

class TizenRendererNuiGL : public TizenRendererGL {
 public:
  explicit TizenRendererNuiGL(TizenViewNui* view_nui, bool enable_impeller);

  virtual ~TizenRendererNuiGL();

  bool CreateSurface(void* render_target,
                     void* render_target_display,
                     int32_t width,
                     int32_t height) override;

  void DestroySurface() override;

  bool OnMakeCurrent() override;

  bool OnClearCurrent() override;

  bool OnMakeResourceCurrent() override;

  bool OnPresent() override;

  uint32_t OnGetFBO() override;

  void* OnProcResolver(const char* name) override;

  bool IsSupportedExtension(const char* name) override;

  void ResizeSurface(int32_t width, int32_t height) override;

  std::unique_ptr<ExternalTexture> CreateExternalTexture(
      const FlutterDesktopTextureInfo* info) override;

 private:
  std::unique_ptr<TizenRendererGL> renderer_;
  TizenViewNui* view_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_NUI_H_
