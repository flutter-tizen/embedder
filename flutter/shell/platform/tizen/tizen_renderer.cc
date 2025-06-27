// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

TizenRenderer::TizenRenderer() {}

bool TizenRenderer::CreateSurface(TizenViewBase* view) {
  TizenGeometry geometry = view->GetGeometry();
  if (dynamic_cast<TizenWindow*>(view)) {
    auto* window = dynamic_cast<TizenWindow*>(view);
    return CreateSurface(window->GetRenderTarget(),
                         window->GetRenderTargetDisplay(), geometry.width,
                         geometry.height);
  } else {
    auto* tizen_view = dynamic_cast<TizenView*>(view);
    return CreateSurface(tizen_view->GetRenderTarget(), nullptr, geometry.width,
                         geometry.height);
  }
}

FlutterTransformation TizenRenderer::GetTransformation() const {
  return flutter_transformation_;
}

FlutterRendererConfig TizenRenderer::GetRendererConfig() {
  FlutterRendererConfig config = {};
  config.type = kOpenGL;
  config.open_gl.struct_size = sizeof(config.open_gl);
  config.open_gl.make_current = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return engine->renderer()->OnMakeCurrent();
  };
  config.open_gl.make_resource_current = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return engine->renderer()->OnMakeResourceCurrent();
  };
  config.open_gl.clear_current = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return engine->renderer()->OnClearCurrent();
  };
  config.open_gl.present = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return engine->renderer()->OnPresent();
  };
  config.open_gl.fbo_callback = [](void* user_data) -> uint32_t {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return engine->renderer()->OnGetFBO();
  };
  config.open_gl.surface_transformation =
      [](void* user_data) -> FlutterTransformation {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return FlutterTransformation();
    }
    return engine->renderer()->GetTransformation();
  };
  config.open_gl.gl_proc_resolver = [](void* user_data,
                                       const char* name) -> void* {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return nullptr;
    }
    return engine->renderer()->OnProcResolver(name);
  };
  config.open_gl.gl_external_texture_frame_callback =
      [](void* user_data, int64_t texture_id, size_t width, size_t height,
         FlutterOpenGLTexture* texture) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->texture_registrar()) {
      return false;
    }
    return engine->texture_registrar()->PopulateTexture(texture_id, width,
                                                        height, texture);
  };
  return config;
}

TizenRenderer::~TizenRenderer() = default;

}  // namespace flutter
