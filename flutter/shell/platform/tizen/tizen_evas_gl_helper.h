// This file is a copy of Tizen's Evas_GL_GLES2_Helpers.h file.
// https://docs.tizen.org/application/native/api/iot-headed/6.0/group__Evas__GL__GLES2__Helpers.html

/**
 * @file Evas_GL_GLES2_Helpers.h
 * @defgroup Evas_GL_GLES2_Helpers Evas GL GLES2 helpers
 * @ingroup Evas_GL
 */

/**
 * @brief Provides a set of helper functions and macros to use GLES 2.0 with
 * @ref Evas_GL "Evas GL".
 *
 * This file redefines all the OpenGL-ES 2.0 functions as follow:
 * @code
#define glFunction  __evas_gl_glapi->glFunction
 * @endcode
 *
 * Extension functions can then be checked for existence simply by writing:
 * @code
if (glExtensionFunction)
  {
     ...
     glExtensionFunction(...);
     ...
  }
 * @endcode
 *
 * When using Elementary @ref GLView, please include the header file
 * @ref Elementary_GL_Helpers "Elementary_GL_Helpers.h" instead.
 *
 * This header file should be included when using @ref Evas_GL "Evas GL"
 * directly at a low level and with an OpenGL-ES 2.0 context only.
 *
 * @note When this file is included, all @c glFunctions are now macros, which
 *       means that the @ref Evas_GL_API struct can't be used anyore.
 *
 * @see @ref elm_opengl_page
 */
#ifndef _EVAS_GL_GLES2_HELPERS_H
#define _EVAS_GL_GLES2_HELPERS_H

#include <Evas_GL.h>

// local convenience macros

/**
 * @addtogroup Evas_GL_GLES2_Helpers
 * @{
 */

/**
 * @brief Macro to place at the beginning of any function using GLES 2.0 APIs
 *
 * Normally, it is necessary to call each function using its pointer as in:
 * @code
glapi->glFunction();
 * @endcode
 *
 * When using this macro, developers can then call all @c glFunctions without
 * changing their code:
 * @code
EVAS_GL_GLES2_USE(evasgl, context); // Add this at the beginning
glFunction(); // All calls 'look' normal
 * @endcode
 *
 * @note Please use @ref ELEMENTARY_GLVIEW_USE() instead, when possible.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
#define EVAS_GL_GLES2_USE(evasgl, context) \
  Evas_GL_API* __evas_gl_glapi = evas_gl_context_api_get(evasgl, context);

/**
 * @brief Macro to place at the beginning of any function using GLES 2.0 APIs
 *
 * This is similar to @ref EVAS_GL_GLES2_USE except that it will return from
 * the function if the GL API can not be used.
 *
 * @note Please use @ref ELEMENTARY_GLVIEW_USE() instead, when possible.
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
#define EVAS_GL_GLES2_USE_OR_RETURN(evasgl, context, retval)               \
  Evas_GL_API* __evas_gl_glapi = evas_gl_context_api_get(evasgl, context); \
  if (!__evas_gl_glapi)                                                    \
    return retval;

// End of the convenience functions

// Global convenience macros

/**
 * @brief Convenience macro to use the GL helpers in simple applications:
declare
 *
 * @c EVAS_GL_GLOBAL_GLES2_DECLARE should be used in a global header for the
 * application. For example, in a platform-specific compatibility header file.
 *
 * To be used with OpenGL-ES 2.0 contexts.
 *
 * Example of a global header file @c main.h:
 * @code
#include <Evas_GL_GLES2_Helpers.h>
// other includes...

EVAS_GL_GLOBAL_GLES2_DECLARE()

// ...
 * @endcode
 *
 * @note Please use @ref ELEMENTARY_GLVIEW_USE() instead, when possible.
 *
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_DECLARE
 * @see @ref EVAS_GL_GLOBAL_GLES2_DEFINE
 * @see @ref EVAS_GL_GLOBAL_GLES2_USE
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
#define EVAS_GL_GLOBAL_GLES2_DECLARE() extern Evas_GL_API* __evas_gl_glapi;

/**
 * @brief Convenience macro to use the GL helpers in simple applications: define
 *
 * To be used with OpenGL-ES 2.0 contexts.
 *
 * Example of a file @c glview.c:
 *
 * @code
#include "main.h"
EVAS_GL_GLOBAL_GLES2_DEFINE()

// ...

static inline void
evgl_init(...)
{
   // ...
   evasgl = evas_gl_new(canvas);
   // ...
   ctx = evas_gl_context_version_create(evasgl, NULL, EVAS_GL_GLES_2_X);
   EVAS_GL_GLOBAL_GLES2_USE(evasgl, ctx);
   // ...
}

// ...
 * @endcode
 *
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_DEFINE
 * @see @ref EVAS_GL_GLOBAL_GLES2_DECLARE
 * @see @ref EVAS_GL_GLOBAL_GLES2_USE
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
#define EVAS_GL_GLOBAL_GLES2_DEFINE() Evas_GL_API* __evas_gl_glapi = NULL;

/**
 * @brief Convenience macro to use the GL helpers in simple applications: use
 *
 * This macro will set the global variable holding the GL API so that it's
 * available to the application.
 *
 * It should be used right after setting up the GL context object.
 *
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_USE
 * @see @ref EVAS_GL_GLOBAL_GLES2_DECLARE
 * @see @ref EVAS_GL_GLOBAL_GLES2_DEFINE
 *
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
#define EVAS_GL_GLOBAL_GLES2_USE(evgl, ctx)               \
  do {                                                    \
    __evas_gl_glapi = evas_gl_context_api_get(evgl, ctx); \
  } while (0)

// End of the convenience functions (global)
/** @} */

#define glActiveTexture __evas_gl_glapi->glActiveTexture
#define glAttachShader __evas_gl_glapi->glAttachShader
#define glBindAttribLocation __evas_gl_glapi->glBindAttribLocation
#define glBindBuffer __evas_gl_glapi->glBindBuffer
#define glBindFramebuffer __evas_gl_glapi->glBindFramebuffer
#define glBindRenderbuffer __evas_gl_glapi->glBindRenderbuffer
#define glBindTexture __evas_gl_glapi->glBindTexture
#define glBlendColor __evas_gl_glapi->glBlendColor
#define glBlendEquation __evas_gl_glapi->glBlendEquation
#define glBlendEquationSeparate __evas_gl_glapi->glBlendEquationSeparate
#define glBlendFunc __evas_gl_glapi->glBlendFunc
#define glBlendFuncSeparate __evas_gl_glapi->glBlendFuncSeparate
#define glBufferData __evas_gl_glapi->glBufferData
#define glBufferSubData __evas_gl_glapi->glBufferSubData
#define glCheckFramebufferStatus __evas_gl_glapi->glCheckFramebufferStatus
#define glClear __evas_gl_glapi->glClear
#define glClearColor __evas_gl_glapi->glClearColor
#define glClearDepthf __evas_gl_glapi->glClearDepthf
#define glClearStencil __evas_gl_glapi->glClearStencil
#define glColorMask __evas_gl_glapi->glColorMask
#define glCompileShader __evas_gl_glapi->glCompileShader
#define glCompressedTexImage2D __evas_gl_glapi->glCompressedTexImage2D
#define glCompressedTexSubImage2D __evas_gl_glapi->glCompressedTexSubImage2D
#define glCopyTexImage2D __evas_gl_glapi->glCopyTexImage2D
#define glCopyTexSubImage2D __evas_gl_glapi->glCopyTexSubImage2D
#define glCreateProgram __evas_gl_glapi->glCreateProgram
#define glCreateShader __evas_gl_glapi->glCreateShader
#define glCullFace __evas_gl_glapi->glCullFace
#define glDeleteBuffers __evas_gl_glapi->glDeleteBuffers
#define glDeleteFramebuffers __evas_gl_glapi->glDeleteFramebuffers
#define glDeleteProgram __evas_gl_glapi->glDeleteProgram
#define glDeleteRenderbuffers __evas_gl_glapi->glDeleteRenderbuffers
#define glDeleteShader __evas_gl_glapi->glDeleteShader
#define glDeleteTextures __evas_gl_glapi->glDeleteTextures
#define glDepthFunc __evas_gl_glapi->glDepthFunc
#define glDepthMask __evas_gl_glapi->glDepthMask
#define glDepthRangef __evas_gl_glapi->glDepthRangef
#define glDetachShader __evas_gl_glapi->glDetachShader
#define glDisable __evas_gl_glapi->glDisable
#define glDisableVertexAttribArray __evas_gl_glapi->glDisableVertexAttribArray
#define glDrawArrays __evas_gl_glapi->glDrawArrays
#define glDrawElements __evas_gl_glapi->glDrawElements
#define glEnable __evas_gl_glapi->glEnable
#define glEnableVertexAttribArray __evas_gl_glapi->glEnableVertexAttribArray
#define glFinish __evas_gl_glapi->glFinish
#define glFlush __evas_gl_glapi->glFlush
#define glFramebufferRenderbuffer __evas_gl_glapi->glFramebufferRenderbuffer
#define glFramebufferTexture2D __evas_gl_glapi->glFramebufferTexture2D
#define glFrontFace __evas_gl_glapi->glFrontFace
#define glGenBuffers __evas_gl_glapi->glGenBuffers
#define glGenerateMipmap __evas_gl_glapi->glGenerateMipmap
#define glGenFramebuffers __evas_gl_glapi->glGenFramebuffers
#define glGenRenderbuffers __evas_gl_glapi->glGenRenderbuffers
#define glGenTextures __evas_gl_glapi->glGenTextures
#define glGetActiveAttrib __evas_gl_glapi->glGetActiveAttrib
#define glGetActiveUniform __evas_gl_glapi->glGetActiveUniform
#define glGetAttachedShaders __evas_gl_glapi->glGetAttachedShaders
#define glGetAttribLocation __evas_gl_glapi->glGetAttribLocation
#define glGetBooleanv __evas_gl_glapi->glGetBooleanv
#define glGetBufferParameteriv __evas_gl_glapi->glGetBufferParameteriv
#define glGetError __evas_gl_glapi->glGetError
#define glGetFloatv __evas_gl_glapi->glGetFloatv
#define glGetFramebufferAttachmentParameteriv \
  __evas_gl_glapi->glGetFramebufferAttachmentParameteriv
#define glGetIntegerv __evas_gl_glapi->glGetIntegerv
#define glGetProgramiv __evas_gl_glapi->glGetProgramiv
#define glGetProgramInfoLog __evas_gl_glapi->glGetProgramInfoLog
#define glGetRenderbufferParameteriv \
  __evas_gl_glapi->glGetRenderbufferParameteriv
#define glGetShaderiv __evas_gl_glapi->glGetShaderiv
#define glGetShaderInfoLog __evas_gl_glapi->glGetShaderInfoLog
#define glGetShaderPrecisionFormat __evas_gl_glapi->glGetShaderPrecisionFormat
#define glGetShaderSource __evas_gl_glapi->glGetShaderSource
#define glGetString __evas_gl_glapi->glGetString
#define glGetTexParameterfv __evas_gl_glapi->glGetTexParameterfv
#define glGetTexParameteriv __evas_gl_glapi->glGetTexParameteriv
#define glGetUniformfv __evas_gl_glapi->glGetUniformfv
#define glGetUniformiv __evas_gl_glapi->glGetUniformiv
#define glGetUniformLocation __evas_gl_glapi->glGetUniformLocation
#define glGetVertexAttribfv __evas_gl_glapi->glGetVertexAttribfv
#define glGetVertexAttribiv __evas_gl_glapi->glGetVertexAttribiv
#define glGetVertexAttribPointerv __evas_gl_glapi->glGetVertexAttribPointerv
#define glHint __evas_gl_glapi->glHint
#define glIsBuffer __evas_gl_glapi->glIsBuffer
#define glIsEnabled __evas_gl_glapi->glIsEnabled
#define glIsFramebuffer __evas_gl_glapi->glIsFramebuffer
#define glIsProgram __evas_gl_glapi->glIsProgram
#define glIsRenderbuffer __evas_gl_glapi->glIsRenderbuffer
#define glIsShader __evas_gl_glapi->glIsShader
#define glIsTexture __evas_gl_glapi->glIsTexture
#define glLineWidth __evas_gl_glapi->glLineWidth
#define glLinkProgram __evas_gl_glapi->glLinkProgram
#define glPixelStorei __evas_gl_glapi->glPixelStorei
#define glPolygonOffset __evas_gl_glapi->glPolygonOffset
#define glReadPixels __evas_gl_glapi->glReadPixels
#define glReleaseShaderCompiler __evas_gl_glapi->glReleaseShaderCompiler
#define glRenderbufferStorage __evas_gl_glapi->glRenderbufferStorage
#define glSampleCoverage __evas_gl_glapi->glSampleCoverage
#define glScissor __evas_gl_glapi->glScissor
#define glShaderBinary __evas_gl_glapi->glShaderBinary
#define glShaderSource __evas_gl_glapi->glShaderSource
#define glStencilFunc __evas_gl_glapi->glStencilFunc
#define glStencilFuncSeparate __evas_gl_glapi->glStencilFuncSeparate
#define glStencilMask __evas_gl_glapi->glStencilMask
#define glStencilMaskSeparate __evas_gl_glapi->glStencilMaskSeparate
#define glStencilOp __evas_gl_glapi->glStencilOp
#define glStencilOpSeparate __evas_gl_glapi->glStencilOpSeparate
#define glTexImage2D __evas_gl_glapi->glTexImage2D
#define glTexParameterf __evas_gl_glapi->glTexParameterf
#define glTexParameterfv __evas_gl_glapi->glTexParameterfv
#define glTexParameteri __evas_gl_glapi->glTexParameteri
#define glTexParameteriv __evas_gl_glapi->glTexParameteriv
#define glTexSubImage2D __evas_gl_glapi->glTexSubImage2D
#define glUniform1f __evas_gl_glapi->glUniform1f
#define glUniform1fv __evas_gl_glapi->glUniform1fv
#define glUniform1i __evas_gl_glapi->glUniform1i
#define glUniform1iv __evas_gl_glapi->glUniform1iv
#define glUniform2f __evas_gl_glapi->glUniform2f
#define glUniform2fv __evas_gl_glapi->glUniform2fv
#define glUniform2i __evas_gl_glapi->glUniform2i
#define glUniform2iv __evas_gl_glapi->glUniform2iv
#define glUniform3f __evas_gl_glapi->glUniform3f
#define glUniform3fv __evas_gl_glapi->glUniform3fv
#define glUniform3i __evas_gl_glapi->glUniform3i
#define glUniform3iv __evas_gl_glapi->glUniform3iv
#define glUniform4f __evas_gl_glapi->glUniform4f
#define glUniform4fv __evas_gl_glapi->glUniform4fv
#define glUniform4i __evas_gl_glapi->glUniform4i
#define glUniform4iv __evas_gl_glapi->glUniform4iv
#define glUniformMatrix2fv __evas_gl_glapi->glUniformMatrix2fv
#define glUniformMatrix3fv __evas_gl_glapi->glUniformMatrix3fv
#define glUniformMatrix4fv __evas_gl_glapi->glUniformMatrix4fv
#define glUseProgram __evas_gl_glapi->glUseProgram
#define glValidateProgram __evas_gl_glapi->glValidateProgram
#define glVertexAttrib1f __evas_gl_glapi->glVertexAttrib1f
#define glVertexAttrib1fv __evas_gl_glapi->glVertexAttrib1fv
#define glVertexAttrib2f __evas_gl_glapi->glVertexAttrib2f
#define glVertexAttrib2fv __evas_gl_glapi->glVertexAttrib2fv
#define glVertexAttrib3f __evas_gl_glapi->glVertexAttrib3f
#define glVertexAttrib3fv __evas_gl_glapi->glVertexAttrib3fv
#define glVertexAttrib4f __evas_gl_glapi->glVertexAttrib4f
#define glVertexAttrib4fv __evas_gl_glapi->glVertexAttrib4fv
#define glVertexAttribPointer __evas_gl_glapi->glVertexAttribPointer
#define glViewport __evas_gl_glapi->glViewport

// GLES 2.0 extensions
#define glGetProgramBinaryOES __evas_gl_glapi->glGetProgramBinaryOES
#define glProgramBinaryOES __evas_gl_glapi->glProgramBinaryOES
#define glMapBufferOES __evas_gl_glapi->glMapBufferOES
#define glUnmapBufferOES __evas_gl_glapi->glUnmapBufferOES
#define glGetBufferPointervOES __evas_gl_glapi->glGetBufferPointervOES
#define glTexImage3DOES __evas_gl_glapi->glTexImage3DOES
#define glTexSubImage3DOES __evas_gl_glapi->glTexSubImage3DOES
#define glCopyTexSubImage3DOES __evas_gl_glapi->glCopyTexSubImage3DOES
#define glCompressedTexImage3DOES __evas_gl_glapi->glCompressedTexImage3DOES
#define glCompressedTexSubImage3DOES \
  __evas_gl_glapi->glCompressedTexSubImage3DOES
#define glFramebufferTexture3DOES __evas_gl_glapi->glFramebufferTexture3DOES
#define glBindVertexArrayOES __evas_gl_glapi->glBindVertexArrayOES
#define glDeleteVertexArraysOES __evas_gl_glapi->glDeleteVertexArraysOES
#define glGenVertexArraysOES __evas_gl_glapi->glGenVertexArraysOES
#define glIsVertexArrayOES __evas_gl_glapi->glIsVertexArrayOES
#define glGetPerfMonitorGroupsAMD __evas_gl_glapi->glGetPerfMonitorGroupsAMD
#define glGetPerfMonitorCountersAMD __evas_gl_glapi->glGetPerfMonitorCountersAMD
#define glGetPerfMonitorGroupStringAMD \
  __evas_gl_glapi->glGetPerfMonitorGroupStringAMD
#define glGetPerfMonitorCounterStringAMD \
  __evas_gl_glapi->glGetPerfMonitorCounterStringAMD
#define glGetPerfMonitorCounterInfoAMD \
  __evas_gl_glapi->glGetPerfMonitorCounterInfoAMD
#define glGenPerfMonitorsAMD __evas_gl_glapi->glGenPerfMonitorsAMD
#define glDeletePerfMonitorsAMD __evas_gl_glapi->glDeletePerfMonitorsAMD
#define glSelectPerfMonitorCountersAMD \
  __evas_gl_glapi->glSelectPerfMonitorCountersAMD
#define glBeginPerfMonitorAMD __evas_gl_glapi->glBeginPerfMonitorAMD
#define glEndPerfMonitorAMD __evas_gl_glapi->glEndPerfMonitorAMD
#define glGetPerfMonitorCounterDataAMD \
  __evas_gl_glapi->glGetPerfMonitorCounterDataAMD
#define glCopyTextureLevelsAPPLE __evas_gl_glapi->glCopyTextureLevelsAPPLE
#define glRenderbufferStorageMultisampleAPPLE \
  __evas_gl_glapi->glRenderbufferStorageMultisampleAPPLE
#define glResolveMultisampleFramebufferAPPLE \
  __evas_gl_glapi->glResolveMultisampleFramebufferAPPLE
#define glFenceSyncAPPLE __evas_gl_glapi->glFenceSyncAPPLE
#define glIsSyncAPPLE __evas_gl_glapi->glIsSyncAPPLE
#define glDeleteSyncAPPLE __evas_gl_glapi->glDeleteSyncAPPLE
#define glClientWaitSyncAPPLE __evas_gl_glapi->glClientWaitSyncAPPLE
#define glWaitSyncAPPLE __evas_gl_glapi->glWaitSyncAPPLE
#define glGetInteger64vAPPLE __evas_gl_glapi->glGetInteger64vAPPLE
#define glGetSyncivAPPLE __evas_gl_glapi->glGetSyncivAPPLE
#define glDiscardFramebufferEXT __evas_gl_glapi->glDiscardFramebufferEXT
#define glMapBufferRangeEXT __evas_gl_glapi->glMapBufferRangeEXT
#define glFlushMappedBufferRangeEXT __evas_gl_glapi->glFlushMappedBufferRangeEXT
#define glMultiDrawArraysEXT __evas_gl_glapi->glMultiDrawArraysEXT
#define glMultiDrawElementsEXT __evas_gl_glapi->glMultiDrawElementsEXT
#define glRenderbufferStorageMultisampleEXT \
  __evas_gl_glapi->glRenderbufferStorageMultisampleEXT
#define glFramebufferTexture2DMultisampleEXT \
  __evas_gl_glapi->glFramebufferTexture2DMultisampleEXT
#define glGetGraphicsResetStatusEXT __evas_gl_glapi->glGetGraphicsResetStatusEXT
#define glReadnPixelsEXT __evas_gl_glapi->glReadnPixelsEXT
#define glGetnUniformfvEXT __evas_gl_glapi->glGetnUniformfvEXT
#define glGetnUniformivEXT __evas_gl_glapi->glGetnUniformivEXT
#define glTexStorage1DEXT __evas_gl_glapi->glTexStorage1DEXT
#define glTexStorage2DEXT __evas_gl_glapi->glTexStorage2DEXT
#define glTexStorage3DEXT __evas_gl_glapi->glTexStorage3DEXT
#define glTextureStorage1DEXT __evas_gl_glapi->glTextureStorage1DEXT
#define glTextureStorage2DEXT __evas_gl_glapi->glTextureStorage2DEXT
#define glTextureStorage3DEXT __evas_gl_glapi->glTextureStorage3DEXT
#define glRenderbufferStorageMultisampleIMG \
  __evas_gl_glapi->glRenderbufferStorageMultisampleIMG
#define glFramebufferTexture2DMultisampleIMG \
  __evas_gl_glapi->glFramebufferTexture2DMultisampleIMG
#define glDeleteFencesNV __evas_gl_glapi->glDeleteFencesNV
#define glGenFencesNV __evas_gl_glapi->glGenFencesNV
#define glIsFenceNV __evas_gl_glapi->glIsFenceNV
#define glTestFenceNV __evas_gl_glapi->glTestFenceNV
#define glGetFenceivNV __evas_gl_glapi->glGetFenceivNV
#define glFinishFenceNV __evas_gl_glapi->glFinishFenceNV
#define glSetFenceNV __evas_gl_glapi->glSetFenceNV
#define glGetDriverControlsQCOM __evas_gl_glapi->glGetDriverControlsQCOM
#define glGetDriverControlStringQCOM \
  __evas_gl_glapi->glGetDriverControlStringQCOM
#define glEnableDriverControlQCOM __evas_gl_glapi->glEnableDriverControlQCOM
#define glDisableDriverControlQCOM __evas_gl_glapi->glDisableDriverControlQCOM
#define glExtGetTexturesQCOM __evas_gl_glapi->glExtGetTexturesQCOM
#define glExtGetBuffersQCOM __evas_gl_glapi->glExtGetBuffersQCOM
#define glExtGetRenderbuffersQCOM __evas_gl_glapi->glExtGetRenderbuffersQCOM
#define glExtGetFramebuffersQCOM __evas_gl_glapi->glExtGetFramebuffersQCOM
#define glExtGetTexLevelParameterivQCOM \
  __evas_gl_glapi->glExtGetTexLevelParameterivQCOM
#define glExtTexObjectStateOverrideiQCOM \
  __evas_gl_glapi->glExtTexObjectStateOverrideiQCOM
#define glExtGetTexSubImageQCOM __evas_gl_glapi->glExtGetTexSubImageQCOM
#define glExtGetBufferPointervQCOM __evas_gl_glapi->glExtGetBufferPointervQCOM
#define glExtGetShadersQCOM __evas_gl_glapi->glExtGetShadersQCOM
#define glExtGetProgramsQCOM __evas_gl_glapi->glExtGetProgramsQCOM
#define glExtIsProgramBinaryQCOM __evas_gl_glapi->glExtIsProgramBinaryQCOM
#define glExtGetProgramBinarySourceQCOM \
  __evas_gl_glapi->glExtGetProgramBinarySourceQCOM
#define glStartTilingQCOM __evas_gl_glapi->glStartTilingQCOM
#define glEndTilingQCOM __evas_gl_glapi->glEndTilingQCOM

// glEvasGL functions
#define glEvasGLImageTargetTexture2DOES \
  __evas_gl_glapi->glEvasGLImageTargetTexture2DOES
#define glEvasGLImageTargetRenderbufferStorageOES \
  __evas_gl_glapi->glEvasGLImageTargetRenderbufferStorageOES

// Evas GL glue layer
#define evasglCreateImage __evas_gl_glapi->evasglCreateImage
#define evasglCreateImageForContext __evas_gl_glapi->evasglCreateImageForContext
#define evasglDestroyImage __evas_gl_glapi->evasglDestroyImage
#define evasglCreateSync __evas_gl_glapi->evasglCreateSync
#define evasglDestroySync __evas_gl_glapi->evasglDestroySync
#define evasglClientWaitSync __evas_gl_glapi->evasglClientWaitSync
#define evasglSignalSync __evas_gl_glapi->evasglSignalSync
#define evasglGetSyncAttrib __evas_gl_glapi->evasglGetSyncAttrib
#define evasglWaitSync __evas_gl_glapi->evasglWaitSync

/**
 * @ingroup Evas_GL_GLES2_Helpers
 * @brief Macro to check that the GL APIs are properly set (GLES 2.0)
 * @since_tizen @if MOBILE 2.3 @elseif WEARABLE 2.3.1 @endif
 */
#define EVAS_GL_GLES2_API_CHECK() \
  ((__evas_gl_glapi != NULL) &&   \
   (__evas_gl_glapi->version == EVAS_GL_API_VERSION) && (glActiveTexture))

/**
 * @}
 */

#endif
