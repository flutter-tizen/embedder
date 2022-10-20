// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_

#include <flutter/plugin_registrar.h>
#include <flutter/standard_message_codec.h>
#include <stdint.h>

#include "flutter_export.h"

class PlatformView {
 public:
  PlatformView(flutter::PluginRegistrar* registrar,
               int view_id,
               void* platform_window)
      : registrar_(registrar), view_id_(view_id) {}

  virtual ~PlatformView() {}

  int GetViewId() { return view_id_; }

  int GetTextureId() { return texture_id_; }

  void SetTextureId(int texture_id) { texture_id_ = texture_id; }

  flutter::PluginRegistrar* GetPluginRegistrar() { return registrar_; }

  virtual void Dispose() = 0;

  virtual void Resize(double width, double height) = 0;
  virtual void Touch(int type,
                     int button,
                     double x,
                     double y,
                     double dx,
                     double dy) = 0;
  virtual void SetDirection(int direction) = 0;

  virtual void ClearFocus() = 0;

  void SetFocus(bool focused) { is_focused_ = focused; }

  bool IsFocused() { return is_focused_; }

  virtual bool SendKey(const char* key,
                       const char* string,
                       const char* compose,
                       uint32_t modifiers,
                       uint32_t scan_code,
                       bool is_down) = 0;

 private:
  flutter::PluginRegistrar* registrar_;
  int view_id_;
  int texture_id_ = 0;
  bool is_focused_ = false;
};

using ByteMessage = std::vector<uint8_t>;

class PlatformViewFactory {
 public:
  PlatformViewFactory(flutter::PluginRegistrar* registrar)
      : registrar_(registrar),
        codec_(flutter::StandardMessageCodec::GetInstance(nullptr)) {}

  virtual ~PlatformViewFactory() {}

  flutter::PluginRegistrar* GetPluginRegistrar() { return registrar_; }

  const flutter::MessageCodec<flutter::EncodableValue>& GetCodec() {
    return codec_;
  }

  virtual PlatformView* Create(int view_id,
                               double width,
                               double height,
                               const ByteMessage& params) = 0;
  virtual void Dispose() = 0;

 private:
  flutter::PluginRegistrar* registrar_;
  const flutter::MessageCodec<flutter::EncodableValue>& codec_;
};

#if defined(__cplusplus)
extern "C" {
#endif

FLUTTER_EXPORT void FlutterDesktopRegisterViewFactory(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* view_type,
    std::unique_ptr<PlatformViewFactory> view_factory);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_PLATFORM_VIEW_H_
