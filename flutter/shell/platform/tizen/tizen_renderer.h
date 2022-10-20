// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_H_
#define EMBEDDER_TIZEN_RENDERER_H_

#include <cstdint>

#include "flutter/shell/platform/tizen/public/flutter_tizen.h"

namespace flutter {

class TizenRenderer {
 public:
  TizenRenderer();

  virtual ~TizenRenderer();

  virtual bool CreateSurface(void* render_target,
                             void* render_target_display,
                             int32_t width,
                             int32_t height) = 0;

  virtual void DestroySurface() = 0;

  bool IsValid() { return is_valid_; }

  FlutterDesktopRendererType type() { return type_; }

  virtual bool OnMakeCurrent() = 0;

  virtual bool OnClearCurrent() = 0;

  virtual bool OnMakeResourceCurrent() = 0;

  virtual bool OnPresent() = 0;

  virtual uint32_t OnGetFBO() = 0;

  virtual void* OnProcResolver(const char* name) = 0;

  virtual bool IsSupportedExtension(const char* name) = 0;

  virtual void ResizeSurface(int32_t width, int32_t height) = 0;

 protected:
  bool is_valid_ = false;

  FlutterDesktopRendererType type_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_H_
