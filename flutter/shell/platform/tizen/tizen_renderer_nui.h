// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_NUI_H_
#define EMBEDDER_TIZEN_RENDERER_NUI_H_

#include <cstdint>
#include <memory>
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#include "flutter/shell/platform/tizen/tizen_view_nui.h"

namespace flutter {

class TizenRendererNui : public TizenRenderer {
 public:
  explicit TizenRendererNui(bool enable_impeller, TizenViewNui* view_nui);

  virtual ~TizenRendererNui();

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

 private:
  std::unique_ptr<TizenRenderer> renderer_;
  TizenViewNui* view_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_NUI_H_
