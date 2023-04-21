// Copyright 2023 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_CURSOR_CHANNEL_H_
#define EMBEDDER_CURSOR_CHANNEL_H_

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

// Channel to set the system's cursor type.
class CursorChannel {
 public:
  explicit CursorChannel(BinaryMessenger* messenger, TizenViewBase* view);
  virtual ~CursorChannel();

 private:
  // Called when a method is called on |channel_|;
  void HandleMethodCall(
      const flutter::MethodCall<EncodableValue>& method_call,
      std::unique_ptr<flutter::MethodResult<EncodableValue>> result);

  // The MethodChannel used for communication with the Flutter engine.
  std::unique_ptr<flutter::MethodChannel<EncodableValue>> channel_;

  // A reference to the native view managed by FlutterTizenView.
  TizenViewBase* view_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_CURSOR_CHANNEL_H_
