// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_PIXEL_EGL_H
#define EMBEDDER_EXTERNAL_TEXTURE_PIXEL_EGL_H

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"

namespace flutter {

class ExternalTexturePixelEGL : public ExternalGLTexture {
 public:
  ExternalTexturePixelEGL(
      FlutterDesktopPixelBufferTextureCallback texture_callback,
      void* user_data);

  ~ExternalTexturePixelEGL() = default;

  bool PopulateOpenGLTexture(size_t width,
                             size_t height,
                             FlutterOpenGLTexture* opengl_texture) override;

  bool CopyPixelBuffer(size_t& width, size_t& height);

 private:
  FlutterDesktopPixelBufferTextureCallback texture_callback_ = nullptr;
  void* user_data_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_EXTERNAL_TEXTURE_PIXEL_EGL_H
