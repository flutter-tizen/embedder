// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_renderer_gl.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

TizenRendererGL::TizenRendererGL() {}

FlutterRendererConfig TizenRendererGL::GetRendererConfig() {
  FlutterRendererConfig config = {};
  config.type = kOpenGL;
  config.open_gl.struct_size = sizeof(config.open_gl);
  config.open_gl.make_current = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return dynamic_cast<TizenRendererGL*>(engine->renderer())->OnMakeCurrent();
  };
  config.open_gl.make_resource_current = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return dynamic_cast<TizenRendererGL*>(engine->renderer())
        ->OnMakeResourceCurrent();
  };
  config.open_gl.clear_current = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return dynamic_cast<TizenRendererGL*>(engine->renderer())->OnClearCurrent();
  };
  config.open_gl.present = [](void* user_data) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return dynamic_cast<TizenRendererGL*>(engine->renderer())->OnPresent();
  };
  config.open_gl.fbo_callback = [](void* user_data) -> uint32_t {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }
    return dynamic_cast<TizenRendererGL*>(engine->renderer())->OnGetFBO();
  };
  config.open_gl.surface_transformation =
      [](void* user_data) -> FlutterTransformation {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return FlutterTransformation();
    }
    return engine->view()->GetFlutterTransformation();
  };
  config.open_gl.gl_proc_resolver = [](void* user_data,
                                       const char* name) -> void* {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return nullptr;
    }
    return dynamic_cast<TizenRendererGL*>(engine->renderer())
        ->OnProcResolver(name);
  };
  config.open_gl.gl_external_texture_frame_callback =
      [](void* user_data, int64_t texture_id, size_t width, size_t height,
         FlutterOpenGLTexture* texture) -> bool {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    if (!engine->texture_registrar()) {
      return false;
    }
    return engine->texture_registrar()->PopulateOpenGLTexture(texture_id, width,
                                                              height, texture);
  };
  return config;
}

ExternalTextureExtensionType
TizenRendererGL::GetExternalTextureExtensionType() {
  ExternalTextureExtensionType gl_extension =
      ExternalTextureExtensionType::kNone;
  if (IsSupportedExtension("EGL_TIZEN_image_native_surface")) {
    gl_extension = ExternalTextureExtensionType::kNativeSurface;
  } else if (IsSupportedExtension("EGL_EXT_image_dma_buf_import")) {
    gl_extension = ExternalTextureExtensionType::kDmaBuffer;
  }
  return gl_extension;
}

TizenRendererGL::~TizenRendererGL() = default;

}  // namespace flutter
