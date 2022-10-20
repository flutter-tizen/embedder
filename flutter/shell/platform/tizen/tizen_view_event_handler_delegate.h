// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_EVENT_HANDLER_DELEGATE_H_
#define EMBEDDER_TIZEN_VIEW_EVENT_HANDLER_DELEGATE_H_

#include <string>

#include "flutter/shell/platform/embedder/embedder.h"

namespace flutter {

class TizenViewEventHandlerDelegate {
 public:
  virtual void OnResize(int32_t left,
                        int32_t top,
                        int32_t width,
                        int32_t height) = 0;

  virtual void OnRotate(int32_t degree) = 0;

  virtual void OnPointerMove(double x,
                             double y,
                             size_t timestamp,
                             FlutterPointerDeviceKind device_kind,
                             int32_t device_id) = 0;

  virtual void OnPointerDown(double x,
                             double y,
                             size_t timestamp,
                             FlutterPointerDeviceKind device_kind,
                             int32_t device_id) = 0;

  virtual void OnPointerUp(double x,
                           double y,
                           size_t timestamp,
                           FlutterPointerDeviceKind device_kind,
                           int32_t device_id) = 0;

  virtual void OnScroll(double x,
                        double y,
                        double delta_x,
                        double delta_y,
                        int scroll_offset_multiplier,
                        size_t timestamp,
                        FlutterPointerDeviceKind device_kind,
                        int32_t device_id) = 0;

  virtual void OnKey(const char* key,
                     const char* string,
                     const char* compose,
                     uint32_t modifiers,
                     uint32_t scan_code,
                     bool is_down) = 0;

  virtual void OnComposeBegin() = 0;

  virtual void OnComposeChange(const std::string& str, int cursor_pos) = 0;

  virtual void OnComposeEnd() = 0;

  virtual void OnCommit(const std::string& str) = 0;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_EVENT_HANDLER_DELEGATE_H_
