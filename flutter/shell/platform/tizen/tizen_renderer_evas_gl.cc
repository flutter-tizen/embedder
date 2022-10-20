// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_evas_gl.h"

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_evas_gl_helper.h"

// g_evas_gl is shared with ExternalTextureSurfaceEGL and
// ExternalTextureSurfaceEvasGL.
Evas_GL* g_evas_gl = nullptr;
EVAS_GL_GLOBAL_GLES2_DEFINE();

namespace flutter {

TizenRendererEvasGL::TizenRendererEvasGL() {
  type_ = FlutterDesktopRendererType::kEvasGL;
}

TizenRendererEvasGL::~TizenRendererEvasGL() {
  DestroySurface();
}

bool TizenRendererEvasGL::CreateSurface(void* render_target,
                                        void* render_target_display,
                                        int32_t width,
                                        int32_t height) {
  evas_gl_ = evas_gl_new(
      evas_object_evas_get(static_cast<Evas_Object*>(render_target)));
  if (!evas_gl_) {
    FT_LOG(Error) << "Could not create an Evas GL object.";
    return false;
  }

  g_evas_gl = evas_gl_;

  gl_config_ = evas_gl_config_new();
  gl_config_->color_format = EVAS_GL_RGBA_8888;
  gl_config_->depth_bits = EVAS_GL_DEPTH_NONE;
  gl_config_->stencil_bits = EVAS_GL_STENCIL_NONE;

  gl_context_ =
      evas_gl_context_version_create(evas_gl_, nullptr, EVAS_GL_GLES_2_X);
  gl_resource_context_ =
      evas_gl_context_version_create(evas_gl_, gl_context_, EVAS_GL_GLES_2_X);

  if (!gl_context_) {
    FT_LOG(Fatal) << "Failed to create an Evas GL context.";
    return false;
  }

  EVAS_GL_GLOBAL_GLES2_USE(evas_gl_, gl_context_);

  gl_surface_ = evas_gl_surface_create(evas_gl_, gl_config_, width, height);
  gl_resource_surface_ = evas_gl_pbuffer_surface_create(evas_gl_, gl_config_,
                                                        width, height, nullptr);

  Evas_Native_Surface native_surface;
  evas_gl_native_surface_get(evas_gl_, gl_surface_, &native_surface);

  image_ = static_cast<Evas_Object*>(render_target);
  evas_object_image_native_surface_set(image_, &native_surface);

  evas_object_image_pixels_get_callback_set(
      image_,
      [](void* data, Evas_Object* o) {
        TizenRendererEvasGL* self = static_cast<TizenRendererEvasGL*>(data);
        if (self->on_pixels_dirty_) {
          self->on_pixels_dirty_();
        }
      },
      this);

  is_valid_ = true;

  // Clear once to remove noise.
  OnMakeCurrent();
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
  OnPresent();

  return true;
}

void TizenRendererEvasGL::DestroySurface() {
  if (evas_gl_) {
    evas_gl_surface_destroy(evas_gl_, gl_surface_);
    evas_gl_surface_destroy(evas_gl_, gl_resource_surface_);

    evas_gl_context_destroy(evas_gl_, gl_context_);
    evas_gl_context_destroy(evas_gl_, gl_resource_context_);

    evas_gl_config_free(gl_config_);
    evas_gl_free(evas_gl_);

    evas_gl_ = nullptr;
  }
}

bool TizenRendererEvasGL::OnMakeCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (evas_gl_make_current(evas_gl_, gl_surface_, gl_context_) != EINA_TRUE) {
    FT_LOG(Error) << "Could not make the onscreen context current.";
    return false;
  }
  return true;
}

bool TizenRendererEvasGL::OnClearCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (evas_gl_make_current(evas_gl_, nullptr, nullptr) != EINA_TRUE) {
    FT_LOG(Error) << "Could not clear the context.";
    return false;
  }
  return true;
}

bool TizenRendererEvasGL::OnMakeResourceCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (evas_gl_make_current(evas_gl_, gl_resource_surface_,
                           gl_resource_context_) != EINA_TRUE) {
    FT_LOG(Error) << "Could not make the offscreen context current.";
    return false;
  }
  return true;
}

bool TizenRendererEvasGL::OnPresent() {
  if (!IsValid()) {
    return false;
  }

  return true;
}

uint32_t TizenRendererEvasGL::OnGetFBO() {
  if (!IsValid()) {
    return 999;
  }
  return 0;
}

bool TizenRendererEvasGL::IsSupportedExtension(const char* name) {
  return strcmp(name, "EGL_TIZEN_image_native_surface") == 0;
}

void TizenRendererEvasGL::MarkPixelsDirty() {
  evas_object_image_pixels_dirty_set(image_, EINA_TRUE);
}

void TizenRendererEvasGL::ResizeSurface(int32_t width, int32_t height) {
  evas_gl_surface_destroy(evas_gl_, gl_surface_);
  evas_gl_surface_destroy(evas_gl_, gl_resource_surface_);

  evas_object_image_native_surface_set(image_, nullptr);
  evas_object_image_size_set(image_, width, height);
  gl_surface_ = evas_gl_surface_create(evas_gl_, gl_config_, width, height);
  gl_resource_surface_ = evas_gl_pbuffer_surface_create(evas_gl_, gl_config_,
                                                        width, height, nullptr);

  Evas_Native_Surface native_surface;
  evas_gl_native_surface_get(evas_gl_, gl_surface_, &native_surface);
  evas_object_image_native_surface_set(image_, &native_surface);
}

const GLubyte* CustomGlGetString(GLenum name) {
  // glGetString in Evas gl doesn't recognize GL_VERSION.
  if (name == GL_VERSION) {
    return reinterpret_cast<const GLubyte*>("OpenGL ES 2.1");
  }
  return glGetString(name);
}

void* TizenRendererEvasGL::OnProcResolver(const char* name) {
  auto address = evas_gl_proc_address_get(evas_gl_, name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
  }
  GL_FUNC(glActiveTexture)
  GL_FUNC(glAttachShader)
  GL_FUNC(glBindAttribLocation)
  GL_FUNC(glBindBuffer)
  GL_FUNC(glBindFramebuffer)
  GL_FUNC(glBindRenderbuffer)
  GL_FUNC(glBindTexture)
  GL_FUNC(glBlendColor)
  GL_FUNC(glBlendEquation)
  GL_FUNC(glBlendEquationSeparate)
  GL_FUNC(glBlendFunc)
  GL_FUNC(glBlendFuncSeparate)
  GL_FUNC(glBufferData)
  GL_FUNC(glBufferSubData)
  GL_FUNC(glCheckFramebufferStatus)
  GL_FUNC(glClear)
  GL_FUNC(glClearColor)
  GL_FUNC(glClearDepthf)
  GL_FUNC(glClearStencil)
  GL_FUNC(glColorMask)
  GL_FUNC(glCompileShader)
  GL_FUNC(glCompressedTexImage2D)
  GL_FUNC(glCompressedTexSubImage2D)
  GL_FUNC(glCopyTexImage2D)
  GL_FUNC(glCopyTexSubImage2D)
  GL_FUNC(glCreateProgram)
  GL_FUNC(glCreateShader)
  GL_FUNC(glCullFace)
  GL_FUNC(glDeleteBuffers)
  GL_FUNC(glDeleteFramebuffers)
  GL_FUNC(glDeleteProgram)
  GL_FUNC(glDeleteRenderbuffers)
  GL_FUNC(glDeleteShader)
  GL_FUNC(glDeleteTextures)
  GL_FUNC(glDepthFunc)
  GL_FUNC(glDepthMask)
  GL_FUNC(glDepthRangef)
  GL_FUNC(glDetachShader)
  GL_FUNC(glDisable)
  GL_FUNC(glDisableVertexAttribArray)
  GL_FUNC(glDrawArrays)
  GL_FUNC(glDrawElements)
  GL_FUNC(glEnable)
  GL_FUNC(glEnableVertexAttribArray)
  GL_FUNC(glFinish)
  GL_FUNC(glFlush)
  GL_FUNC(glFramebufferRenderbuffer)
  GL_FUNC(glFramebufferTexture2D)
  GL_FUNC(glFrontFace)
  GL_FUNC(glGenBuffers)
  GL_FUNC(glGenerateMipmap)
  GL_FUNC(glGenFramebuffers)
  GL_FUNC(glGenRenderbuffers)
  GL_FUNC(glGenTextures)
  GL_FUNC(glGetActiveAttrib)
  GL_FUNC(glGetActiveUniform)
  GL_FUNC(glGetAttachedShaders)
  GL_FUNC(glGetAttribLocation)
  GL_FUNC(glGetBooleanv)
  GL_FUNC(glGetBufferParameteriv)
  GL_FUNC(glGetError)
  GL_FUNC(glGetFloatv)
  GL_FUNC(glGetFramebufferAttachmentParameteriv)
  GL_FUNC(glGetIntegerv)
  GL_FUNC(glGetProgramiv)
  GL_FUNC(glGetProgramInfoLog)
  GL_FUNC(glGetRenderbufferParameteriv)
  GL_FUNC(glGetShaderiv)
  GL_FUNC(glGetShaderInfoLog)
  GL_FUNC(glGetShaderPrecisionFormat)
  GL_FUNC(glGetShaderSource)
  // GL_FUNC(glGetString)
  else if (strcmp(name, "glGetString") == 0) {
    return reinterpret_cast<void*>(CustomGlGetString);
  }
  GL_FUNC(glGetTexParameterfv)
  GL_FUNC(glGetTexParameteriv)
  GL_FUNC(glGetUniformfv)
  GL_FUNC(glGetUniformiv)
  GL_FUNC(glGetUniformLocation)
  GL_FUNC(glGetVertexAttribfv)
  GL_FUNC(glGetVertexAttribiv)
  GL_FUNC(glGetVertexAttribPointerv)
  GL_FUNC(glHint)
  GL_FUNC(glIsBuffer)
  GL_FUNC(glIsEnabled)
  GL_FUNC(glIsFramebuffer)
  GL_FUNC(glIsProgram)
  GL_FUNC(glIsRenderbuffer)
  GL_FUNC(glIsShader)
  GL_FUNC(glIsTexture)
  GL_FUNC(glLineWidth)
  GL_FUNC(glLinkProgram)
  GL_FUNC(glPixelStorei)
  GL_FUNC(glPolygonOffset)
  GL_FUNC(glReadPixels)
  GL_FUNC(glReleaseShaderCompiler)
  GL_FUNC(glRenderbufferStorage)
  GL_FUNC(glSampleCoverage)
  GL_FUNC(glScissor)
  GL_FUNC(glShaderBinary)
  GL_FUNC(glShaderSource)
  GL_FUNC(glStencilFunc)
  GL_FUNC(glStencilFuncSeparate)
  GL_FUNC(glStencilMask)
  GL_FUNC(glStencilMaskSeparate)
  GL_FUNC(glStencilOp)
  GL_FUNC(glStencilOpSeparate)
  GL_FUNC(glTexImage2D)
  GL_FUNC(glTexParameterf)
  GL_FUNC(glTexParameterfv)
  GL_FUNC(glTexParameteri)
  GL_FUNC(glTexParameteriv)
  GL_FUNC(glTexSubImage2D)
  GL_FUNC(glUniform1f)
  GL_FUNC(glUniform1fv)
  GL_FUNC(glUniform1i)
  GL_FUNC(glUniform1iv)
  GL_FUNC(glUniform2f)
  GL_FUNC(glUniform2fv)
  GL_FUNC(glUniform2i)
  GL_FUNC(glUniform2iv)
  GL_FUNC(glUniform3f)
  GL_FUNC(glUniform3fv)
  GL_FUNC(glUniform3i)
  GL_FUNC(glUniform3iv)
  GL_FUNC(glUniform4f)
  GL_FUNC(glUniform4fv)
  GL_FUNC(glUniform4i)
  GL_FUNC(glUniform4iv)
  GL_FUNC(glUniformMatrix2fv)
  GL_FUNC(glUniformMatrix3fv)
  GL_FUNC(glUniformMatrix4fv)
  GL_FUNC(glUseProgram)
  GL_FUNC(glValidateProgram)
  GL_FUNC(glVertexAttrib1f)
  GL_FUNC(glVertexAttrib1fv)
  GL_FUNC(glVertexAttrib2f)
  GL_FUNC(glVertexAttrib2fv)
  GL_FUNC(glVertexAttrib3f)
  GL_FUNC(glVertexAttrib3fv)
  GL_FUNC(glVertexAttrib4f)
  GL_FUNC(glVertexAttrib4fv)
  GL_FUNC(glVertexAttribPointer)
  GL_FUNC(glViewport)
  GL_FUNC(glGetProgramBinaryOES)
  GL_FUNC(glProgramBinaryOES)
  GL_FUNC(glMapBufferOES)
  GL_FUNC(glUnmapBufferOES)
  GL_FUNC(glGetBufferPointervOES)
  GL_FUNC(glTexImage3DOES)
  GL_FUNC(glTexSubImage3DOES)
  GL_FUNC(glCopyTexSubImage3DOES)
  GL_FUNC(glCompressedTexImage3DOES)
  GL_FUNC(glCompressedTexSubImage3DOES)
  GL_FUNC(glFramebufferTexture3DOES)
  GL_FUNC(glBindVertexArrayOES)
  GL_FUNC(glDeleteVertexArraysOES)
  GL_FUNC(glGenVertexArraysOES)
  GL_FUNC(glIsVertexArrayOES)
  GL_FUNC(glGetPerfMonitorGroupsAMD)
  GL_FUNC(glGetPerfMonitorCountersAMD)
  GL_FUNC(glGetPerfMonitorGroupStringAMD)
  GL_FUNC(glGetPerfMonitorCounterStringAMD)
  GL_FUNC(glGetPerfMonitorCounterInfoAMD)
  GL_FUNC(glGenPerfMonitorsAMD)
  GL_FUNC(glDeletePerfMonitorsAMD)
  GL_FUNC(glSelectPerfMonitorCountersAMD)
  GL_FUNC(glBeginPerfMonitorAMD)
  GL_FUNC(glEndPerfMonitorAMD)
  GL_FUNC(glGetPerfMonitorCounterDataAMD)
  GL_FUNC(glCopyTextureLevelsAPPLE)
  GL_FUNC(glRenderbufferStorageMultisampleAPPLE)
  GL_FUNC(glResolveMultisampleFramebufferAPPLE)
  GL_FUNC(glFenceSyncAPPLE)
  GL_FUNC(glIsSyncAPPLE)
  GL_FUNC(glDeleteSyncAPPLE)
  GL_FUNC(glClientWaitSyncAPPLE)
  GL_FUNC(glWaitSyncAPPLE)
  GL_FUNC(glGetInteger64vAPPLE)
  GL_FUNC(glGetSyncivAPPLE)
  GL_FUNC(glDiscardFramebufferEXT)
  GL_FUNC(glMapBufferRangeEXT)
  GL_FUNC(glFlushMappedBufferRangeEXT)
  GL_FUNC(glMultiDrawArraysEXT)
  GL_FUNC(glMultiDrawElementsEXT)
  GL_FUNC(glRenderbufferStorageMultisampleEXT)
  GL_FUNC(glFramebufferTexture2DMultisampleEXT)
  GL_FUNC(glGetGraphicsResetStatusEXT)
  GL_FUNC(glReadnPixelsEXT)
  GL_FUNC(glGetnUniformfvEXT)
  GL_FUNC(glGetnUniformivEXT)
  GL_FUNC(glTexStorage1DEXT)
  GL_FUNC(glTexStorage2DEXT)
  GL_FUNC(glTexStorage3DEXT)
  GL_FUNC(glTextureStorage1DEXT)
  GL_FUNC(glTextureStorage2DEXT)
  GL_FUNC(glTextureStorage3DEXT)
  GL_FUNC(glRenderbufferStorageMultisampleIMG)
  GL_FUNC(glFramebufferTexture2DMultisampleIMG)
  GL_FUNC(glDeleteFencesNV)
  GL_FUNC(glGenFencesNV)
  GL_FUNC(glIsFenceNV)
  GL_FUNC(glTestFenceNV)
  GL_FUNC(glGetFenceivNV)
  GL_FUNC(glFinishFenceNV)
  GL_FUNC(glSetFenceNV)
  GL_FUNC(glGetDriverControlsQCOM)
  GL_FUNC(glGetDriverControlStringQCOM)
  GL_FUNC(glEnableDriverControlQCOM)
  GL_FUNC(glDisableDriverControlQCOM)
  GL_FUNC(glExtGetTexturesQCOM)
  GL_FUNC(glExtGetBuffersQCOM)
  GL_FUNC(glExtGetRenderbuffersQCOM)
  GL_FUNC(glExtGetFramebuffersQCOM)
  GL_FUNC(glExtGetTexLevelParameterivQCOM)
  GL_FUNC(glExtTexObjectStateOverrideiQCOM)
  GL_FUNC(glExtGetTexSubImageQCOM)
  GL_FUNC(glExtGetBufferPointervQCOM)
  GL_FUNC(glExtGetShadersQCOM)
  GL_FUNC(glExtGetProgramsQCOM)
  GL_FUNC(glExtIsProgramBinaryQCOM)
  GL_FUNC(glExtGetProgramBinarySourceQCOM)
  GL_FUNC(glStartTilingQCOM)
  GL_FUNC(glEndTilingQCOM)
  GL_FUNC(glEvasGLImageTargetTexture2DOES)
  GL_FUNC(glEvasGLImageTargetRenderbufferStorageOES)
#undef GL_FUNC

  FT_LOG(Warn) << "Could not resolve: " << name;
  return nullptr;
}

}  // namespace flutter
