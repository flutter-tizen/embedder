// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_texture_registrar.h"

#include <iostream>
#include <mutex>

#include "flutter/shell/platform/tizen/external_texture_pixel_egl.h"
#include "flutter/shell/platform/tizen/external_texture_pixel_egl_impeller.h"
#include "flutter/shell/platform/tizen/external_texture_pixel_evas_gl.h"
#include "flutter/shell/platform/tizen/external_texture_surface_egl.h"
#include "flutter/shell/platform/tizen/external_texture_surface_egl_impeller.h"
#include "flutter/shell/platform/tizen/external_texture_surface_evas_gl.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_renderer_evas_gl.h"

namespace flutter {

FlutterTizenTextureRegistrar::FlutterTizenTextureRegistrar(
    FlutterTizenEngine* engine)
    : engine_(engine) {}

int64_t FlutterTizenTextureRegistrar::RegisterTexture(
    const FlutterDesktopTextureInfo* texture_info) {
  if (texture_info->type != kFlutterDesktopPixelBufferTexture &&
      texture_info->type != kFlutterDesktopGpuSurfaceTexture) {
    FT_LOG(Error) << "Attempted to register texture of unsupported type.";
    return -1;
  }

  if (texture_info->type == kFlutterDesktopPixelBufferTexture) {
    if (!texture_info->pixel_buffer_config.callback) {
      FT_LOG(Error) << "Invalid pixel buffer texture callback.";
      return -1;
    }
  }

  if (texture_info->type == kFlutterDesktopGpuSurfaceTexture) {
    if (!texture_info->gpu_surface_config.callback) {
      FT_LOG(Error) << "Invalid GPU surface texture callback.";
      return -1;
    }
  }
  std::unique_ptr<ExternalTexture> texture_gl =
      engine_->renderer()->CreateExternalTexture(texture_info);
  if (!texture_gl) {
    FT_LOG(Error) << "Failed to create ExternalTexture.";
    return -1;
  }
  int64_t texture_id = texture_gl->TextureId();

  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    textures_[texture_id] = std::move(texture_gl);
  }

  engine_->RegisterExternalTexture(texture_id);
  return texture_id;
}

bool FlutterTizenTextureRegistrar::UnregisterTexture(int64_t texture_id) {
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto iter = textures_.find(texture_id);
    if (iter == textures_.end()) {
      return false;
    }
    textures_.erase(iter);
  }
  return engine_->UnregisterExternalTexture(texture_id);
}

bool FlutterTizenTextureRegistrar::MarkTextureFrameAvailable(
    int64_t texture_id) {
  return engine_->MarkExternalTextureFrameAvailable(texture_id);
}

bool FlutterTizenTextureRegistrar::PopulateTexture(
    int64_t texture_id,
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  ExternalTexture* texture;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto iter = textures_.find(texture_id);
    if (iter == textures_.end()) {
      return false;
    }
    texture = iter->second.get();
  }
  return texture->PopulateTexture(width, height, opengl_texture);
}

}  // namespace flutter
