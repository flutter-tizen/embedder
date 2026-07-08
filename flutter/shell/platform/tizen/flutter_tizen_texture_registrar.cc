// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_texture_registrar.h"

#include <memory>
#include <mutex>

#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/external_texture_surface_egl.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_renderer_gl.h"

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
  std::unique_ptr<ExternalTexture> texture_gl = nullptr;

#ifndef UNIT_TESTS
  if (engine_->renderer()) {
    texture_gl = engine_->renderer()->CreateExternalTexture(texture_info);
  }
#else
  texture_gl = std::make_unique<ExternalTextureSurfaceEGL>(
      ExternalTextureExtensionType::kDmaBuffer,
      texture_info->gpu_surface_config.callback,
      texture_info->gpu_surface_config.user_data);
#endif

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

bool FlutterTizenTextureRegistrar::UnregisterTexture(int64_t texture_id,
                                                     void (*callback)(void*),
                                                     void* user_data) {
  std::unique_ptr<ExternalTexture> texture;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto iter = textures_.find(texture_id);
    if (iter == textures_.end()) {
      return false;
    }
    // Remove from the map first so no *new* PopulateGLTexture() lookup can
    // find this texture. A PopulateGLTexture() already in flight on the render
    // thread may still hold a raw pointer to it (it drops map_mutex_ before
    // using the texture), so destruction is deferred to a render-thread task
    // below.
    texture = std::move(iter->second);
    textures_.erase(iter);
  }

  // Destroy the texture on the render thread rather than here on the calling
  // (platform) thread. The engine runs this task only after any in-flight
  // frame callback on the render thread has completed, so it cannot race with
  // a PopulateGLTexture() that is still reading this texture. Tearing it down
  // on the platform thread instead would let the backing TBM buffer be freed
  // while the render thread still has it mapped/referenced -> heap corruption
  // and the "tbm_bo_free ... lock_cnt" crash. Running on the render thread
  // also puts glDeleteTextures on the thread that owns the GL context.
  //
  // The completion |callback| (used by webview_flutter_lwe to free its TBM
  // buffer pool) is likewise invoked only after the render thread is done with
  // the texture, giving the plugin a correct "safe to free" signal.
  FlutterTizenEngine* engine = engine_;
  auto* gl_renderer = dynamic_cast<TizenRendererGL*>(engine->renderer());
  // shared_ptr (not unique_ptr): PostRenderThreadTask takes a copyable
  // std::function, so the captured owner must be copyable.
  std::shared_ptr<ExternalTexture> tex(std::move(texture));
  engine->PostRenderThreadTask(
      [engine, gl_renderer, texture_id, tex, callback, user_data]() mutable {
        // On the render thread, make the render context current so
        // glDeleteTextures in the texture's destructor targets the correct
        // context (the engine does not guarantee a current context when
        // running posted tasks).
        //
        // If the engine has shut down between PostRenderThreadTask() being
        // called and this task running, PostRenderThreadTask() runs the task
        // inline and the renderer may already be destroyed, so skip
        // OnMakeCurrent() to avoid touching a dangling GL context/renderer.
        // UnregisterExternalTexture() stays unconditional: it is null-safe at
        // the embedder boundary (a torn-down engine handle just yields an
        // error) and is the registrar's core teardown step.
        if (engine->IsRunning() && gl_renderer) {
          gl_renderer->OnMakeCurrent();
        }
        tex.reset();
        engine->UnregisterExternalTexture(texture_id);
        if (callback) {
          callback(user_data);
        }
      });
  return true;
}

bool FlutterTizenTextureRegistrar::MarkTextureFrameAvailable(
    int64_t texture_id) {
  return engine_->MarkExternalTextureFrameAvailable(texture_id);
}

bool FlutterTizenTextureRegistrar::PopulateGLTexture(
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
  return dynamic_cast<ExternalGLTexture*>(texture)->PopulateGLTexture(
      width, height, opengl_texture);
}

bool FlutterTizenTextureRegistrar::PopulateVulkanTexture(
    int64_t texture_id,
    size_t width,
    size_t height,
    FlutterVulkanTexture* vulkan_texture) {
  ExternalTexture* texture;
  {
    std::lock_guard<std::mutex> lock(map_mutex_);
    auto iter = textures_.find(texture_id);
    if (iter == textures_.end()) {
      return false;
    }
    texture = iter->second.get();
  }
  return dynamic_cast<ExternalVulkanTexture*>(texture)->PopulateVulkanTexture(
      width, height, vulkan_texture);
}

}  // namespace flutter
