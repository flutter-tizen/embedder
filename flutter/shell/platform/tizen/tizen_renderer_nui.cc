// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_renderer_nui.h"
#include "flutter/shell/platform/tizen/tizen_renderer_egl.h"
#include "tizen_renderer.h"

namespace flutter {

TizenRendererNui::TizenRendererNui(bool enable_impeller, TizenViewNui* view_nui)
    : renderer_(std::make_unique<TizenRendererEgl>(enable_impeller)),
      view_(view_nui) {}

TizenRendererNui::~TizenRendererNui() {}

bool TizenRendererNui::CreateSurface(void* render_target,
                                     void* render_target_display,
                                     int32_t width,
                                     int32_t height) {
  return renderer_->CreateSurface(render_target, render_target_display, width,
                                  height);
}

void TizenRendererNui::DestroySurface() {
  renderer_->DestroySurface();
}

bool TizenRendererNui::OnMakeCurrent() {
  return renderer_->OnMakeCurrent();
}

bool TizenRendererNui::OnClearCurrent() {
  return renderer_->OnClearCurrent();
}

bool TizenRendererNui::OnMakeResourceCurrent() {
  return renderer_->OnMakeResourceCurrent();
}

bool TizenRendererNui::OnPresent() {
  auto result = renderer_->OnPresent();
  view_->RequestRendering();
  return result;
}

uint32_t TizenRendererNui::OnGetFBO() {
  return renderer_->OnGetFBO();
}

void* TizenRendererNui::OnProcResolver(const char* name) {
  return renderer_->OnProcResolver(name);
}

bool TizenRendererNui::IsSupportedExtension(const char* name) {
  return renderer_->IsSupportedExtension(name);
}

void TizenRendererNui::ResizeSurface(int32_t width, int32_t height) {
  renderer_->ResizeSurface(width, height);
}

}  // namespace flutter
