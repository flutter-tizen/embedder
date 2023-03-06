// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_EGL_H_
#define EMBEDDER_TIZEN_RENDERER_EGL_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <array>
#include <list>
#include <string>
#include <unordered_map>

#include "flutter/shell/platform/embedder/embedder.h"
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

  bool OnPresent(const FlutterPresentInfo* info) override;

  uint32_t OnGetFBO() override;

  void* OnProcResolver(const char* name) override;

  bool IsSupportedExtension(const char* name) override;

  void ResizeSurface(int32_t width, int32_t height) override;

  void PopulateExistingDamage(const intptr_t fbo_id,
                              FlutterDamage* existing_damage);

 private:
  bool ChooseEGLConfiguration();

  void PrintEGLError();

  // Auxiliary function used to transform a FlutterRect into the format that is
  // expected by the EGL functions (i.e. array of EGLint).
  std::array<EGLint, 4> RectToInts(const FlutterRect rect);

  EGLConfig egl_config_ = nullptr;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;
  EGLContext egl_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  EGLContext egl_resource_context_ = EGL_NO_CONTEXT;
  EGLSurface egl_resource_surface_ = EGL_NO_SURFACE;
  std::string egl_extension_str_;

  PFNEGLSETDAMAGEREGIONKHRPROC egl_set_damage_region_ = nullptr;
  PFNEGLSWAPBUFFERSWITHDAMAGEKHRPROC egl_swap_buffers_with_damage_ = nullptr;

  // Keeps track of the most recent frame damages so that existing damage can
  // be easily computed.
  std::list<FlutterRect> damage_history_;

  // Keeps track of the existing damage associated with each FBO ID.
  std::unordered_map<intptr_t, FlutterRect*> existing_damage_map_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_EGL_H_
