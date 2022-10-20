// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_LIFECYCLE_CHANNEL_H_
#define EMBEDDER_LIFECYCLE_CHANNEL_H_

#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"

namespace flutter {

class LifecycleChannel {
 public:
  explicit LifecycleChannel(BinaryMessenger* messenger);
  virtual ~LifecycleChannel();

  void AppIsInactive();
  void AppIsResumed();
  void AppIsPaused();
  void AppIsDetached();

 private:
  std::unique_ptr<BasicMessageChannel<EncodableValue>> channel_;
};

}  // namespace flutter

#endif  // EMBEDDER_LIFECYCLE_CHANNEL_H_
