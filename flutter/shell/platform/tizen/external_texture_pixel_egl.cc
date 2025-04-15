// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_pixel_egl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

namespace flutter {

bool ExternalTexturePixelEGL::PopulateTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  if (!CopyPixelBuffer(width, height)) {
    return false;
  }

  // Populate the texture object used by the engine.
  opengl_texture->target = GL_TEXTURE_2D;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8_OES;
  opengl_texture->destruction_callback = nullptr;
  opengl_texture->user_data = nullptr;
  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}

ExternalTexturePixelEGL::ExternalTexturePixelEGL(
    FlutterDesktopPixelBufferTextureCallback texture_callback,
    void* user_data)
    : ExternalTexture(),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

bool ExternalTexturePixelEGL::CopyPixelBuffer(size_t& width, size_t& height) {
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

  if (state_->gl_texture == 0) {
    glGenTextures(1, static_cast<GLuint*>(&state_->gl_texture));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(state_->gl_texture));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER_OES);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER_OES);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(state_->gl_texture));
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixel_buffer->width,
               pixel_buffer->height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               pixel_buffer->buffer);
  return true;
}

}  // namespace flutter
