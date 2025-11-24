// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "external_texture_surface_egl_impeller.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <type_traits>
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

ExternalTextureSurfaceEGLImpeller::ExternalTextureSurfaceEGLImpeller(
    ExternalTextureExtensionType gl_extension,
    FlutterDesktopGpuSurfaceTextureCallback texture_callback,
    void* user_data)
    : ExternalGLTexture(gl_extension),
      texture_callback_(texture_callback),
      user_data_(user_data) {}

ExternalTextureSurfaceEGLImpeller::~ExternalTextureSurfaceEGLImpeller() {
  ReleaseImage();
}

bool ExternalTextureSurfaceEGLImpeller::PopulateOpenGLTexture(
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

  if (!CreateOrUpdateEglImage(gpu_surface)) {
    FT_LOG(Info) << "CreateOrUpdateEglImage fail for texture ID: "
                 << texture_id_;
    return false;
  }

  opengl_texture->bind_callback = OnBindCallback;
  opengl_texture->destruction_callback = nullptr;
  opengl_texture->user_data = this;
  opengl_texture->width = width;
  opengl_texture->height = height;
  return true;
}

bool ExternalTextureSurfaceEGLImpeller::CreateOrUpdateEglImage(
    const FlutterDesktopGpuSurfaceDescriptor* descriptor) {
  if (descriptor == nullptr || descriptor->handle == nullptr) {
    ReleaseImage();
    return false;
  }
  void* handle = descriptor->handle;
  if (handle != last_surface_handle_) {
    ReleaseImage();

    const tbm_surface_h tbm_surface =
        reinterpret_cast<tbm_surface_h>(descriptor->handle);

    tbm_surface_info_s info;
    if (tbm_surface_get_info(tbm_surface, &info) != TBM_SURFACE_ERROR_NONE) {
      if (descriptor->release_callback) {
        descriptor->release_callback(descriptor->release_context);
      }
      return false;
    }

    PFNEGLCREATEIMAGEKHRPROC n_eglCreateImageKHR =
        reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(
            eglGetProcAddress("eglCreateImageKHR"));

    if (state_->gl_extension == ExternalTextureExtensionType::kNativeSurface) {
      const EGLint attribs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE,
                                EGL_NONE};
      egl_src_image_ =
          n_eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                              EGL_NATIVE_SURFACE_TIZEN, tbm_surface, attribs);
    } else if (state_->gl_extension ==
               ExternalTextureExtensionType::kDmaBuffer) {
      EGLint attribs[50];
      int atti = 0;
      int plane_fd_ext[4] = {
          EGL_DMA_BUF_PLANE0_FD_EXT, EGL_DMA_BUF_PLANE1_FD_EXT,
          EGL_DMA_BUF_PLANE2_FD_EXT, EGL_DMA_BUF_PLANE3_FD_EXT};
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
        attribs[atti++] = static_cast<int>(reinterpret_cast<size_t>(
            tbm_bo_get_handle(tbo, TBM_DEVICE_3D).ptr));
        attribs[atti++] = plane_offset_ext[i];
        attribs[atti++] = info.planes[i].offset;
        attribs[atti++] = plane_pitch_ext[i];
        attribs[atti++] = info.planes[i].stride;
      }
      attribs[atti++] = EGL_NONE;
      egl_src_image_ =
          n_eglCreateImageKHR(eglGetCurrentDisplay(), EGL_NO_CONTEXT,
                              EGL_LINUX_DMA_BUF_EXT, nullptr, attribs);
    }
    if (!egl_src_image_) {
      if (state_->gl_extension != ExternalTextureExtensionType::kNone) {
        FT_LOG(Error) << "eglCreateImageKHR failed with an error "
                      << eglGetError() << " for texture ID: " << texture_id_;
      } else {
        FT_LOG(Error) << "Either EGL_TIZEN_image_native_surface or "
                         "EGL_EXT_image_dma_buf_import shoule be supported.";
      }
      if (descriptor->release_callback) {
        descriptor->release_callback(descriptor->release_context);
      }
      return false;
    }
    last_surface_handle_ = handle;
  }
  if (descriptor->release_callback) {
    descriptor->release_callback(descriptor->release_context);
  }
  return true;
}

void ExternalTextureSurfaceEGLImpeller::ReleaseImage() {
  if (egl_src_image_) {
    PFNEGLDESTROYIMAGEKHRPROC n_eglDestroyImageKHR =
        reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
            eglGetProcAddress("eglDestroyImageKHR"));
    n_eglDestroyImageKHR(eglGetCurrentDisplay(), egl_src_image_);
    egl_src_image_ = nullptr;
  }
}

bool ExternalTextureSurfaceEGLImpeller::OnBindCallback(void* user_data) {
  ExternalTextureSurfaceEGLImpeller* self =
      static_cast<ExternalTextureSurfaceEGLImpeller*>(user_data);
  return self->OnBind();
}

bool ExternalTextureSurfaceEGLImpeller::OnBind() {
  if (!egl_src_image_) {
    return false;
  }
  PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES =
      reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
          eglGetProcAddress("glEGLImageTargetTexture2DOES"));
  glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_src_image_);
  return true;
}

}  // namespace flutter
