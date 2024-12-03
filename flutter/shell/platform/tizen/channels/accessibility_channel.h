// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ACCESSIBILITY_CHANNEL_H_
#define EMBEDDER_ACCESSIBILITY_CHANNEL_H_

#include <Eldbus.h>

#include <functional>
#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"

namespace flutter {

class AccessibilityChannel {
 public:
  explicit AccessibilityChannel(BinaryMessenger* messenger);
  virtual ~AccessibilityChannel();

 private:
  std::unique_ptr<BasicMessageChannel<EncodableValue>> channel_;

  Eldbus_Connection* session_bus_ = nullptr;
  Eldbus_Connection* accessibility_bus_ = nullptr;
  Eldbus_Object* bus_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_ACCESSIBILITY_CHANNEL_H_
