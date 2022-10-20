// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_EGL_H_
#define EMBEDDER_TIZEN_RENDERER_EGL_H_

#include <EGL/egl.h>

#include <string>

#include "flutter/shell/platform/tizen/tizen_renderer.h"

namespace flutter {

class TizenRendererEgl : public TizenRenderer {
 public:
  explicit TizenRendererEgl();

  virtual ~TizenRendererEgl();

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
  bool ChooseEGLConfiguration();

  void PrintEGLError();

  EGLConfig egl_config_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;

  std::string egl_extension_str_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_EGL_H_
