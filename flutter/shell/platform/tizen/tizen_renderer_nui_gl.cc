// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_renderer_nui_gl.h"
#include "flutter/shell/platform/tizen/tizen_renderer_egl.h"
#include "tizen_renderer.h"

namespace flutter {

TizenRendererNuiGL::TizenRendererNuiGL(TizenViewNui* view_nui,
                                       bool enable_impeller)
    : renderer_(std::make_unique<TizenRendererEgl>(view_nui, enable_impeller)),
      view_(view_nui) {}

TizenRendererNuiGL::~TizenRendererNuiGL() {}

bool TizenRendererNuiGL::CreateSurface(void* render_target,
                                       void* render_target_display,
                                       int32_t width,
                                       int32_t height) {
  return renderer_->CreateSurface(render_target, render_target_display, width,
                                  height);
}

void TizenRendererNuiGL::DestroySurface() {
  renderer_->DestroySurface();
}

bool TizenRendererNuiGL::OnMakeCurrent() {
  return renderer_->OnMakeCurrent();
}

bool TizenRendererNuiGL::OnClearCurrent() {
  return renderer_->OnClearCurrent();
}

bool TizenRendererNuiGL::OnMakeResourceCurrent() {
  return renderer_->OnMakeResourceCurrent();
}

bool TizenRendererNuiGL::OnPresent() {
  auto result = renderer_->OnPresent();
  view_->RequestRendering();
  return result;
}

uint32_t TizenRendererNuiGL::OnGetFBO() {
  return renderer_->OnGetFBO();
}

void* TizenRendererNuiGL::OnProcResolver(const char* name) {
  return renderer_->OnProcResolver(name);
}

bool TizenRendererNuiGL::IsSupportedExtension(const char* name) {
  return renderer_->IsSupportedExtension(name);
}

void TizenRendererNuiGL::ResizeSurface(int32_t width, int32_t height) {
  renderer_->ResizeSurface(width, height);
}

}  // namespace flutter
