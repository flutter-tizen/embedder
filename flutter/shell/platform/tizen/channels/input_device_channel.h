// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_INPUT_DEVICE_CHANNEL_H_
#define EMBEDDER_INPUT_DEVICE_CHANNEL_H_

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"

namespace flutter {

class InputDeviceChannel {
 public:
  explicit InputDeviceChannel(BinaryMessenger* messenger);
  virtual ~InputDeviceChannel();

  void SetLastKeyboardInfo(const std::string& name) {
    last_keyboard_name_ = name;
  }

 private:
  void HandleMethodCall(const MethodCall<EncodableValue>& method_call,
                        std::unique_ptr<MethodResult<EncodableValue>> result);

  std::unique_ptr<MethodChannel<EncodableValue>> channel_;

  // The name of the last keyboard device used.
  std::string last_keyboard_name_;
};

}  // namespace flutter

#endif  // EMBEDDER_INPUT_DEVICE_CHANNEL_H_
