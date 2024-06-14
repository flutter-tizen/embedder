// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_PIXEL_EGL_IMPELLER_H
#define EMBEDDER_EXTERNAL_TEXTURE_PIXEL_EGL_IMPELLER_H

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"

namespace flutter {

class ExternalTexturePixelEGLImpeller : public ExternalTexture {
 public:
  ExternalTexturePixelEGLImpeller(
      FlutterDesktopPixelBufferTextureCallback texture_callback,
      void* user_data);

  ~ExternalTexturePixelEGLImpeller() = default;

  bool PopulateTexture(size_t width,
                       size_t height,
                       FlutterOpenGLTexture* opengl_texture) override;

 private:
  FlutterDesktopPixelBufferTextureCallback texture_callback_ = nullptr;
  void* user_data_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_EXTERNAL_TEXTURE_PIXEL_EGL_H
