// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_
#define EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_

#include <functional>

#include <Elementary.h>

#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#include "flutter/shell/platform/tizen/tizen_renderer_gl.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

using OnPixelsDirty = std::function<void()>;

class TizenRendererEvasGL : public TizenRendererGL {
 public:
  explicit TizenRendererEvasGL(TizenViewBase* view_base);

  virtual ~TizenRendererEvasGL();

  bool OnMakeCurrent() override;

  bool OnClearCurrent() override;

  bool OnMakeResourceCurrent() override;

  bool OnPresent() override;

  uint32_t OnGetFBO() override;

  void* OnProcResolver(const char* name) override;

  bool IsSupportedExtension(const char* name) override;

  void SetOnPixelsDirty(OnPixelsDirty callback) { on_pixels_dirty_ = callback; }

  void MarkPixelsDirty();

  void ResizeSurface(int32_t width, int32_t height) override;

  std::unique_ptr<ExternalTexture> CreateExternalTexture(
      const FlutterDesktopTextureInfo* info) override;

 protected:
  bool CreateSurface(void* render_target,
                     void* render_target_display,
                     int32_t width,
                     int32_t height) override;

  void DestroySurface() override;

 private:
  Evas_GL* evas_gl_ = nullptr;
  Evas_GL_Config* gl_config_ = nullptr;
  Evas_GL_Context* gl_context_ = nullptr;
  Evas_GL_Context* gl_resource_context_ = nullptr;
  Evas_GL_Surface* gl_surface_ = nullptr;
  Evas_GL_Surface* gl_resource_surface_ = nullptr;

  Evas_Object* image_ = nullptr;
  OnPixelsDirty on_pixels_dirty_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_EVAS_GL_H_
