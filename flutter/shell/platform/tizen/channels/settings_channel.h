// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_SETTINGS_CHANNEL_H_
#define EMBEDDER_SETTINGS_CHANNEL_H_

#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "rapidjson/document.h"

namespace flutter {

class SettingsChannel {
 public:
  explicit SettingsChannel(BinaryMessenger* messenger);
  virtual ~SettingsChannel();

 private:
  void SendSettingsEvent();
  bool SetUpLocaleTimeFormat();
  float SetUpTextScaleFactor();

  std::unique_ptr<BasicMessageChannel<rapidjson::Document>> channel_;
  bool locale_time_format_ = false;
  float text_scale_factor_ = 1.0;
};

}  // namespace flutter

#endif  // EMBEDDER_SETTINGS_CHANNEL_H_
