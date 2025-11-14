// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_SURFACE_EGL_H_
#define EMBEDDER_EXTERNAL_TEXTURE_SURFACE_EGL_H_

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"

namespace flutter {

class ExternalTextureSurfaceEGL : public ExternalGLTexture {
 public:
  ExternalTextureSurfaceEGL(
      ExternalTextureExtensionType gl_extension,
      FlutterDesktopGpuSurfaceTextureCallback texture_callback,
      void* user_data);

  virtual ~ExternalTextureSurfaceEGL();

  // Accepts texture buffer copy request from the Flutter engine.
  // When the user side marks the texture_id as available, the Flutter engine
  // will callback to this method and ask to populate the |opengl_texture|
  // object, such as the texture type and the format of the pixel buffer and the
  // texture object.
  //
  // Returns true on success, false on failure.
  bool PopulateOpenGLTexture(size_t width,
                             size_t height,
                             FlutterOpenGLTexture* opengl_texture) override;

 private:
  FlutterDesktopGpuSurfaceTextureCallback texture_callback_ = nullptr;
  void* user_data_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_EXTERNAL_TEXTURE_SURFACE_EGL_H_
