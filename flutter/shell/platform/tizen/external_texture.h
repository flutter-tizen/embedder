// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_H_
#define EMBEDDER_EXTERNAL_TEXTURE_H_

#include <atomic>
#include <memory>

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

static std::atomic<int64_t> next_texture_id = {1};

class ExternalTexture {
 public:
  ExternalTexture() : texture_id_(next_texture_id++) {}

  virtual ~ExternalTexture() = default;

  // Returns the unique id for the ExternalTextureGL instance.
  int64_t TextureId() { return texture_id_; }

 protected:
  const int64_t texture_id_ = 0;
};

enum class ExternalTextureExtensionType { kNone, kNativeSurface, kDmaBuffer };

struct ExternalTextureGLState {
  uint32_t gl_texture;
  ExternalTextureExtensionType gl_extension;
};

class ExternalGLTexture : public ExternalTexture {
 public:
  ExternalGLTexture(ExternalTextureExtensionType gl_extension =
                        ExternalTextureExtensionType::kNone)
      : ExternalTexture(), state_(std::make_unique<ExternalTextureGLState>()) {
    state_->gl_extension = gl_extension;
  }

  virtual bool PopulateOpenGLTexture(size_t width,
                                     size_t height,
                                     FlutterOpenGLTexture* opengl_texture) = 0;

 protected:
  std::unique_ptr<ExternalTextureGLState> state_;
};

class ExternalVulkanTexture : public ExternalTexture {
 public:
  ExternalVulkanTexture() : ExternalTexture() {}

  virtual bool PopulateVulkanTexture(size_t width,
                                     size_t height,
                                     FlutterVulkanTexture* vulkan_texture) = 0;
};

}  // namespace flutter

#endif  // EMBEDDER_EXTERNAL_TEXTURE_H_
