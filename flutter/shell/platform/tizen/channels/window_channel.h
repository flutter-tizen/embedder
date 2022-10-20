// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_WINDOW_CHANNEL_H_
#define EMBEDDER_WINDOW_CHANNEL_H_

#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

// Channel to get/set application's window size and device's screen size.
class WindowChannel {
 public:
  explicit WindowChannel(BinaryMessenger* messenger, TizenWindow* window);
  virtual ~WindowChannel();

 private:
  void HandleMethodCall(const MethodCall<EncodableValue>& method_call,
                        std::unique_ptr<MethodResult<EncodableValue>> result);

  std::unique_ptr<MethodChannel<EncodableValue>> channel_;

  // A reference to the native window managed by FlutterTizenView.
  TizenWindow* window_;
};

}  // namespace flutter

#endif  // EMBEDDER_WINDOW_CHANNEL_H_
