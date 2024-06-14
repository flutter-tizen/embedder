// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_EXTERNAL_TEXTURE_SURFACE_EGL_IMPELLER_H_
#define EMBEDDER_EXTERNAL_TEXTURE_SURFACE_EGL_IMPELLER_H_

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace flutter {

class ExternalTextureSurfaceEGLImpeller : public ExternalTexture {
 public:
  ExternalTextureSurfaceEGLImpeller(
      ExternalTextureExtensionType gl_extension,
      FlutterDesktopGpuSurfaceTextureCallback texture_callback,
      void* user_data);

  virtual ~ExternalTextureSurfaceEGLImpeller();

  // Accepts texture buffer copy request from the Flutter engine.
  // When the user side marks the texture_id as available, the Flutter engine
  // will callback to this method and ask to populate the |opengl_texture|
  // object, such as the texture type and the format of the pixel buffer and the
  // texture object.
  //
  // Returns true on success, false on failure.
  bool PopulateTexture(size_t width,
                       size_t height,
                       FlutterOpenGLTexture* opengl_texture) override;

 private:
  static bool OnBindCallback(void* user_data);
  static void OnDestruction(void* user_data);
  bool CreateOrUpdateEglImage(
      const FlutterDesktopGpuSurfaceDescriptor* descriptor);
  void ReleaseImage();
  bool OnBind();
  FlutterDesktopGpuSurfaceTextureCallback texture_callback_ = nullptr;
  void* user_data_ = nullptr;
  EGLImageKHR egl_src_image_ = nullptr;
  void* last_surface_handle_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_EXTERNAL_TEXTURE_SURFACE_EGL_H_
