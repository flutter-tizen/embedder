// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_surface_evas_gl.h"

#include <tbm_surface.h>

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_evas_gl_helper.h"

extern Evas_GL* g_evas_gl;
EVAS_GL_GLOBAL_GLES2_DECLARE();

namespace flutter {

ExternalTextureSurfaceEvasGL::ExternalTextureSurfaceEvasGL(
    ExternalTextureExtensionType gl_extension,
    FlutterDesktopGpuSurfaceTextureCallback texture_callback,
    void* user_data)
    : ExternalTexture(gl_extension),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

ExternalTextureSurfaceEvasGL::~ExternalTextureSurfaceEvasGL() {
  if (state_->gl_texture != 0) {
    glDeleteTextures(1, static_cast<GLuint*>(&state_->gl_texture));
  }
  state_.release();
}

bool ExternalTextureSurfaceEvasGL::PopulateTexture(
    size_t width,
    size_t height,
    FlutterOpenGLTexture* opengl_texture) {
  if (!texture_callback_) {
    return false;
  }
  const FlutterDesktopGpuSurfaceDescriptor* gpu_surface =
      texture_callback_(width, height, user_data_);
  if (!gpu_surface) {
    FT_LOG(Info) << "gpu_surface is null for texture ID: " << texture_id_;
    return false;
  }

  if (!gpu_surface->handle) {
    FT_LOG(Info) << "tbm_surface is null for texture ID: " << texture_id_;
    if (gpu_surface->release_callback) {
      gpu_surface->release_callback(gpu_surface->release_context);
    }
    return false;
  }
  const tbm_surface_h tbm_surface =
      reinterpret_cast<tbm_surface_h>(gpu_surface->handle);

  tbm_surface_info_s info;
  if (tbm_surface_get_info(tbm_surface, &info) != TBM_SURFACE_ERROR_NONE) {
    FT_LOG(Info) << "tbm_surface is invalid for texture ID: " << texture_id_;
    if (gpu_surface->release_callback) {
      gpu_surface->release_callback(gpu_surface->release_context);
    }
    return false;
  }

  EvasGLImage evasgl_src_image = nullptr;
  if (state_->gl_extension == ExternalTextureExtensionType::kNativeSurface) {
    int attribs[] = {EVAS_GL_IMAGE_PRESERVED, GL_TRUE, 0};
    evasgl_src_image = evasglCreateImageForContext(
        g_evas_gl, evas_gl_current_context_get(g_evas_gl),
        EVAS_GL_NATIVE_SURFACE_TIZEN, tbm_surface, attribs);
  } else if (state_->gl_extension == ExternalTextureExtensionType::kDmaBuffer) {
    FT_LOG(Error)
        << "EGL_EXT_image_dma_buf_import is not supported this renderer.";
    if (gpu_surface->release_callback) {
      gpu_surface->release_callback(gpu_surface->release_context);
    }
    return false;
  }
  if (!evasgl_src_image) {
    if (gpu_surface->release_callback) {
      gpu_surface->release_callback(gpu_surface->release_context);
    }
    return false;
  }
  if (state_->gl_texture == 0) {
    glGenTextures(1, static_cast<GLuint*>(&state_->gl_texture));
    glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                  static_cast<GLuint>(state_->gl_texture));
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S,
                    GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                  static_cast<GLuint>(state_->gl_texture));
  }
  glEvasGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, evasgl_src_image);
  if (evasgl_src_image) {
    evasglDestroyImage(evasgl_src_image);
  }

  opengl_texture->target = GL_TEXTURE_EXTERNAL_OES;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8;
  opengl_texture->destruction_callback = nullptr;
  opengl_texture->user_data = nullptr;
  opengl_texture->width = width;
  opengl_texture->height = height;
  if (gpu_surface->release_callback) {
    gpu_surface->release_callback(gpu_surface->release_context);
  }
  return true;
}

}  // namespace flutter
