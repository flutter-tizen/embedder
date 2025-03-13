// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_surface_egl.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#ifndef EGL_DMA_BUF_PLANE3_FD_EXT
#define EGL_DMA_BUF_PLANE3_FD_EXT 0x3440
#endif
#ifndef EGL_DMA_BUF_PLANE3_OFFSET_EXT
#define EGL_DMA_BUF_PLANE3_OFFSET_EXT 0x3441
#endif
#ifndef EGL_DMA_BUF_PLANE3_PITCH_EXT
#define EGL_DMA_BUF_PLANE3_PITCH_EXT 0x3442
#endif

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

ExternalTextureSurfaceEGL::ExternalTextureSurfaceEGL(
    ExternalTextureExtensionType gl_extension,
    FlutterDesktopGpuSurfaceTextureCallback texture_callback,
    void* user_data)
    : ExternalTexture(gl_extension),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

ExternalTextureSurfaceEGL::~ExternalTextureSurfaceEGL() {
  if (state_->gl_texture != 0) {
    glDeleteTextures(1, static_cast<GLuint*>(&state_->gl_texture));
  }
}

bool ExternalTextureSurfaceEGL::PopulateTexture(
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

  PFNEGLCREATEIMAGEKHRPROC n_eglCreateImageKHR =
      reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
          eglGetProcAddress("eglCreateImageKHR"));
  EGLImageKHR egl_src_image = nullptr;

  if (state_->gl_extension == ExternalTextureExtensionType::kNativeSurface) {
    const EGLint attribs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,
                              EGL_NONE};
    egl_src_image =
        n_eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                            EGL_NATIVE_SURFACE_TIZEN, tbm_surface, attribs);
  } else if (state_->gl_extension == ExternalTextureExtensionType::kDmaBuffer) {
    EGLint attribs[50];
    int atti = 0;
    int plane_fd_ext[4] = {EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE1_FD_EXT,
                           EGL_DMA_BUF_PLANE2_FD_EXT,
                           EGL_DMA_BUF_PLANE3_FD_EXT};
    int plane_offset_ext[4] = {
        EGL_DMA_BUF_PLANE0_OFFSET_EXT, EGL_DMA_BUF_PLANE1_OFFSET_EXT,
        EGL_DMA_BUF_PLANE2_OFFSET_EXT, EGL_DMA_BUF_PLANE3_OFFSET_EXT};
    int plane_pitch_ext[4] = {
        EGL_DMA_BUF_PLANE0_PITCH_EXT, EGL_DMA_BUF_PLANE1_PITCH_EXT,
        EGL_DMA_BUF_PLANE2_PITCH_EXT, EGL_DMA_BUF_PLANE3_PITCH_EXT};

    attribs[atti++] = EGL_WIDTH;
    attribs[atti++] = info.width;
    attribs[atti++] = EGL_HEIGHT;
    attribs[atti++] = info.height;
    attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
    attribs[atti++] = info.format;

    int num_planes = tbm_surface_internal_get_num_planes(info.format);
    for (int i = 0; i < num_planes; i++) {
      int bo_idx = tbm_surface_internal_get_plane_bo_idx(tbm_surface, i);
      tbm_bo tbo = tbm_surface_internal_get_bo(tbm_surface, bo_idx);
      attribs[atti++] = plane_fd_ext[i];
      attribs[atti++] = static_cast<int>(
          reinterpret_cast<size_t>(tbm_bo_get_handle(tbo, TBM_DEVICE_3D).ptr));
      attribs[atti++] = plane_offset_ext[i];
      attribs[atti++] = info.planes[i].offset;
      attribs[atti++] = plane_pitch_ext[i];
      attribs[atti++] = info.planes[i].stride;
    }
    attribs[atti++] = EGL_NONE;
    egl_src_image =
        n_eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                            EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
  }

  if (!egl_src_image) {
    if (state_->gl_extension != ExternalTextureExtensionType::kNone) {
      FT_LOG(Error) << "eglCreateImageKHR failed with an error "
                    << eglGetError() << " for texture ID: " << texture_id_;
    } else {
      FT_LOG(Error) << "Either EGL_TIZEN_image_native_surface or "
                       "EGL_EXT_image_dma_buf_import shoule be supported.";
    }
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
                    GL_CLAMP_TO_BORDER_OES);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T,
                    GL_CLAMP_TO_BORDER_OES);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    glBindTexture(GL_TEXTURE_EXTERNAL_OES,
                  static_cast<GLuint>(state_->gl_texture));
  }
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
      reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
          eglGetProcAddress("glEGLImageTargetTexture2DOES"));
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_src_image);
  if (egl_src_image) {
    PFNEGLDESTROYIMAGEKHRPROC n_eglDestroyImageKHR =
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
            eglGetProcAddress("eglDestroyImageKHR"));
    n_eglDestroyImageKHR(eglGetCurrentDisplay(), egl_src_image);
  }
  opengl_texture->target = GL_TEXTURE_EXTERNAL_OES;
  opengl_texture->name = state_->gl_texture;
  opengl_texture->format = GL_RGBA8_OES;
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
