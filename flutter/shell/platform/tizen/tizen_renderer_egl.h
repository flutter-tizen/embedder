// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_EGL_H_
#define EMBEDDER_TIZEN_RENDERER_EGL_H_

#include <EGL/egl.h>

#include <string>

#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#include "flutter/shell/platform/tizen/tizen_renderer_gl.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

class TizenRendererEgl : public TizenRendererGL {
 public:
  explicit TizenRendererEgl(TizenViewBase* view_base, bool enable_impeller);

  virtual ~TizenRendererEgl();

  virtual bool OnMakeCurrent() override;

  virtual bool OnClearCurrent() override;

  virtual bool OnMakeResourceCurrent() override;

  virtual bool OnPresent() override;

  virtual uint32_t OnGetFBO() override;

  virtual void* OnProcResolver(const char* name) override;

  virtual bool IsSupportedExtension(const char* name) override;

  virtual void ResizeSurface(int32_t width, int32_t height) override;

  virtual std::unique_ptr<ExternalTexture> CreateExternalTexture(
      const FlutterDesktopTextureInfo* texture_info) override;

 protected:
  bool CreateSurface(void* render_target,
                     void* render_target_display,
                     int32_t width,
                     int32_t height) override;

  void DestroySurface() override;

 private:
  bool ChooseEGLConfiguration();

  void PrintEGLError();

  EGLConfig egl_config_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;

  std::string egl_extension_str_;
  bool enable_impeller_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_EGL_H_
