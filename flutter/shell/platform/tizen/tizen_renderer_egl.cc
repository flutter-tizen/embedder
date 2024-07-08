// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_renderer_egl.h"

#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#ifdef NUI_SUPPORT
#include <dali/devel-api/adaptor-framework/native-image-source-queue.h>
#endif
#include <tbm_dummy_display.h>
#include <tbm_surface.h>
#include <tbm_surface_queue.h>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

TizenRendererEgl::TizenRendererEgl(bool enable_impeller)
    : enable_impeller_(enable_impeller) {}

TizenRendererEgl::~TizenRendererEgl() {
  DestroySurface();
}

bool TizenRendererEgl::CreateSurface(void* render_target,
                                     void* render_target_display,
                                     int32_t width,
                                     int32_t height) {
  if (render_target_display) {
    egl_display_ =
        eglGetDisplay(static_cast<wl_display*>(render_target_display));
  } else {
    egl_display_ = eglGetDisplay(tbm_dummy_display_create());
  }

  if (egl_display_ == EGL_NO_DISPLAY) {
    PrintEGLError();
    FT_LOG(Error) << "Could not get EGL display.";
    return false;
  }

  if (!ChooseEGLConfiguration()) {
    FT_LOG(Error) << "Could not choose an EGL configuration.";
    return false;
  }

  egl_extension_str_ = eglQueryString(egl_display_, EGL_EXTENSIONS);

  {
    const EGLint attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

    egl_context_ =
        eglCreateContext(egl_display_, egl_config_, EGL_NO_CONTEXT, attribs);
    if (egl_context_ == EGL_NO_CONTEXT) {
      PrintEGLError();
      FT_LOG(Error) << "Could not create an onscreen context.";
      return false;
    }

    egl_resource_context_ =
        eglCreateContext(egl_display_, egl_config_, egl_context_, attribs);
    if (egl_resource_context_ == EGL_NO_CONTEXT) {
      PrintEGLError();
      FT_LOG(Error) << "Could not create an offscreen context.";
      return false;
    }
  }

  {
    const EGLint attribs[] = {EGL_NONE};

    if (render_target_display) {
      auto* egl_window =
          static_cast<EGLNativeWindowType*>(ecore_wl2_egl_window_native_get(
              static_cast<Ecore_Wl2_Egl_Window*>(render_target)));
      egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_,
                                            egl_window, attribs);
    } else {
#ifdef NUI_SUPPORT
      Dali::NativeImageSourceQueuePtr dali_native_image_queue =
          static_cast<Dali::NativeImageSourceQueue*>(render_target);
      tbm_surface_queue_h tbm_surface_queue_ =
          Dali::AnyCast<tbm_surface_queue_h>(
              dali_native_image_queue->GetNativeImageSourceQueue());
      auto* egl_window =
          reinterpret_cast<EGLNativeWindowType*>(tbm_surface_queue_);
      egl_surface_ = eglCreateWindowSurface(egl_display_, egl_config_,
                                            egl_window, attribs);
#endif
    }

    if (egl_surface_ == EGL_NO_SURFACE) {
      FT_LOG(Error) << "Could not create an onscreen window surface.";
      return false;
    }
  }

  {
    const EGLint attribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};

    egl_resource_surface_ =
        eglCreatePbufferSurface(egl_display_, egl_config_, attribs);
    if (egl_resource_surface_ == EGL_NO_SURFACE) {
      FT_LOG(Error) << "Could not create an offscreen window surface.";
      return false;
    }
  }

  is_valid_ = true;
  return true;
}

void TizenRendererEgl::DestroySurface() {
  if (egl_display_) {
    eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);

    if (EGL_NO_SURFACE != egl_surface_) {
      eglDestroySurface(egl_display_, egl_surface_);
      egl_surface_ = EGL_NO_SURFACE;
    }

    if (EGL_NO_CONTEXT != egl_context_) {
      eglDestroyContext(egl_display_, egl_context_);
      egl_context_ = EGL_NO_CONTEXT;
    }

    if (EGL_NO_SURFACE != egl_resource_surface_) {
      eglDestroySurface(egl_display_, egl_resource_surface_);
      egl_resource_surface_ = EGL_NO_SURFACE;
    }

    if (EGL_NO_CONTEXT != egl_resource_context_) {
      eglDestroyContext(egl_display_, egl_resource_context_);
      egl_resource_context_ = EGL_NO_CONTEXT;
    }

    eglTerminate(egl_display_);
    egl_display_ = EGL_NO_DISPLAY;
  }
}

bool TizenRendererEgl::ChooseEGLConfiguration() {
  if (!eglInitialize(egl_display_, nullptr, nullptr)) {
    PrintEGLError();
    FT_LOG(Error) << "Could not initialize the EGL display.";
    return false;
  }

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    PrintEGLError();
    FT_LOG(Error) << "Could not bind the ES API.";
    return false;
  }

  EGLint config_size = 0;
  if (!eglGetConfigs(egl_display_, nullptr, 0, &config_size)) {
    PrintEGLError();
    FT_LOG(Error) << "Could not query framebuffer configurations.";
    return false;
  }

  EGLConfig* configs = (EGLConfig*)calloc(config_size, sizeof(EGLConfig));
  EGLint num_config;
  if (enable_impeller_) {
    EGLint impeller_config_attribs[] = {
        // clang-format off
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLE_BUFFERS,  1,
        EGL_SAMPLES,         4,
        EGL_STENCIL_SIZE,    8,
        EGL_DEPTH_SIZE,      0,
        EGL_NONE
        // clang-format on
    };
    if (!eglChooseConfig(egl_display_, impeller_config_attribs, configs,
                         config_size, &num_config)) {
      free(configs);
      PrintEGLError();
      FT_LOG(Error) << "No matching configurations found.";
      return false;
    }
  } else {
    EGLint config_attribs[] = {
        // clang-format off
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_ALPHA_SIZE,      EGL_DONT_CARE,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SAMPLE_BUFFERS,  EGL_DONT_CARE,
        EGL_SAMPLES,         EGL_DONT_CARE,
        EGL_NONE
        // clang-format on
    };
    if (!eglChooseConfig(egl_display_, config_attribs, configs, config_size,
                         &num_config)) {
      free(configs);
      PrintEGLError();
      FT_LOG(Error) << "No matching configurations found.";
      return false;
    }
  }

  int buffer_size = 32;
  EGLint size;
  for (int i = 0; i < num_config; i++) {
    eglGetConfigAttrib(egl_display_, configs[i], EGL_BUFFER_SIZE, &size);
    if (buffer_size == size) {
      egl_config_ = configs[i];
      break;
    }
  }
  free(configs);
  if (!egl_config_) {
    FT_LOG(Error) << "No matching configuration found.";
    return false;
  }

  return true;
}

bool TizenRendererEgl::OnMakeCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_surface_, egl_surface_, egl_context_) !=
      EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not make the onscreen context current.";
    return false;
  }
  return true;
}

bool TizenRendererEgl::OnClearCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT) != EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not clear the context.";
    return false;
  }
  return true;
}

bool TizenRendererEgl::OnMakeResourceCurrent() {
  if (!IsValid()) {
    return false;
  }
  if (eglMakeCurrent(egl_display_, egl_resource_surface_, egl_resource_surface_,
                     egl_resource_context_) != EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not make the offscreen context current.";
    return false;
  }
  return true;
}

bool TizenRendererEgl::OnPresent() {
  if (!IsValid()) {
    return false;
  }

  if (eglSwapBuffers(egl_display_, egl_surface_) != EGL_TRUE) {
    PrintEGLError();
    FT_LOG(Error) << "Could not swap EGL buffers.";
    return false;
  }
  return true;
}

uint32_t TizenRendererEgl::OnGetFBO() {
  if (!IsValid()) {
    return 999;
  }
  return 0;
}

void TizenRendererEgl::PrintEGLError() {
  EGLint error = eglGetError();
  switch (error) {
#define CASE_PRINT(value)                     \
  case value: {                               \
    FT_LOG(Error) << "EGL error: " << #value; \
    break;                                    \
  }
    CASE_PRINT(EGL_NOT_INITIALIZED)
    CASE_PRINT(EGL_BAD_ACCESS)
    CASE_PRINT(EGL_BAD_ALLOC)
    CASE_PRINT(EGL_BAD_ATTRIBUTE)
    CASE_PRINT(EGL_BAD_CONTEXT)
    CASE_PRINT(EGL_BAD_CONFIG)
    CASE_PRINT(EGL_BAD_CURRENT_SURFACE)
    CASE_PRINT(EGL_BAD_DISPLAY)
    CASE_PRINT(EGL_BAD_SURFACE)
    CASE_PRINT(EGL_BAD_MATCH)
    CASE_PRINT(EGL_BAD_PARAMETER)
    CASE_PRINT(EGL_BAD_NATIVE_PIXMAP)
    CASE_PRINT(EGL_BAD_NATIVE_WINDOW)
    CASE_PRINT(EGL_CONTEXT_LOST)
#undef CASE_PRINT
    default: {
      FT_LOG(Error) << "Unknown EGL error: " << error;
    }
  }
}

bool TizenRendererEgl::IsSupportedExtension(const char* name) {
  return strstr(egl_extension_str_.c_str(), name);
}

void TizenRendererEgl::ResizeSurface(int32_t width, int32_t height) {
  // Do nothing.
}

void* TizenRendererEgl::OnProcResolver(const char* name) {
  auto address = eglGetProcAddress(name);
  if (address != nullptr) {
    return reinterpret_cast<void*>(address);
  }
#define GL_FUNC(FunctionName)                     \
  else if (strcmp(name, #FunctionName) == 0) {    \
    return reinterpret_cast<void*>(FunctionName); \
  }
  GL_FUNC(eglGetCurrentDisplay)
  GL_FUNC(eglQueryString)
  GL_FUNC(glActiveTexture)
  GL_FUNC(glAttachShader)
  GL_FUNC(glBindAttribLocation)
  GL_FUNC(glBindBuffer)
  GL_FUNC(glBindFramebuffer)
  GL_FUNC(glBindRenderbuffer)
  GL_FUNC(glBindTexture)
  GL_FUNC(glBlendColor)
  GL_FUNC(glBlendEquation)
  GL_FUNC(glBlendFunc)
  GL_FUNC(glBufferData)
  GL_FUNC(glBufferSubData)
  GL_FUNC(glCheckFramebufferStatus)
  GL_FUNC(glClear)
  GL_FUNC(glClearColor)
  GL_FUNC(glClearStencil)
  GL_FUNC(glColorMask)
  GL_FUNC(glCompileShader)
  GL_FUNC(glCompressedTexImage2D)
  GL_FUNC(glCompressedTexSubImage2D)
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
  GL_FUNC(glDepthMask)
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
  GL_FUNC(glGetBufferParameteriv)
  GL_FUNC(glGetError)
  GL_FUNC(glGetFloatv)
  GL_FUNC(glGetFramebufferAttachmentParameteriv)
  GL_FUNC(glGetIntegerv)
  GL_FUNC(glGetProgramInfoLog)
  GL_FUNC(glGetProgramiv)
  GL_FUNC(glGetRenderbufferParameteriv)
  GL_FUNC(glGetShaderInfoLog)
  GL_FUNC(glGetShaderiv)
  GL_FUNC(glGetShaderPrecisionFormat)
  GL_FUNC(glGetString)
  GL_FUNC(glGetUniformLocation)
  GL_FUNC(glIsTexture)
  GL_FUNC(glLineWidth)
  GL_FUNC(glLinkProgram)
  GL_FUNC(glPixelStorei)
  GL_FUNC(glReadPixels)
  GL_FUNC(glRenderbufferStorage)
  GL_FUNC(glScissor)
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
  GL_FUNC(glVertexAttrib1f)
  GL_FUNC(glVertexAttrib2fv)
  GL_FUNC(glVertexAttrib3fv)
  GL_FUNC(glVertexAttrib4fv)
  GL_FUNC(glVertexAttribPointer)
  GL_FUNC(glViewport)
#undef GL_FUNC

  FT_LOG(Warn) << "Could not resolve: " << name;
  return nullptr;
}
}  // namespace flutter
