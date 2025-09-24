// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_pixel_egl_impeller.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

bool ExternalTexturePixelEGLImpeller::PopulateOpenGLTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  if (!texture_callback_) {
    return false;
  }

  const FlutterDesktopPixelBuffer* pixel_buffer =
      texture_callback_(width, height, user_data_);

  if (!pixel_buffer || !pixel_buffer->buffer) {
    return false;
  }

  width = pixel_buffer->width;
  height = pixel_buffer->height;

  // Populate the texture object used by the engine.
  opengl_texture->buffer = pixel_buffer->buffer;
  opengl_texture->buffer_size =
      size_t(pixel_buffer->width) * size_t(pixel_buffer->height) * 4;
  opengl_texture->destruction_callback = nullptr;
  opengl_texture->user_data = nullptr;
  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}

ExternalTexturePixelEGLImpeller::ExternalTexturePixelEGLImpeller(
    FlutterDesktopPixelBufferTextureCallback texture_callback,
    void* user_data)
    : ExternalGLTexture(),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

}  // namespace flutter
