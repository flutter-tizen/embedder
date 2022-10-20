// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_channel.h"

#include <Elementary.h>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/accessibility";

}  // namespace

AccessibilityChannel::AccessibilityChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMessageCodec::GetInstance())) {
  channel_->SetMessageHandler([&](const auto& message, auto reply) {
    if (std::holds_alternative<EncodableMap>(message)) {
      auto map = std::get<EncodableMap>(message);
      EncodableValueHolder<std::string> type(&map, "type");
      EncodableValueHolder<EncodableMap> data(&map, "data");

      if (type) {
        FT_LOG(Info) << "Received " << *type << " message.";
        if (*type == "announce" && data) {
          EncodableValueHolder<std::string> msg(data.value, "message");
          if (msg) {
            elm_atspi_bridge_utils_say(msg->c_str(), true, nullptr, nullptr);
          }
        }
      }
    }
    reply(EncodableValue());
  });
}

AccessibilityChannel::~AccessibilityChannel() {
  channel_->SetMessageHandler(nullptr);
}

}  // namespace flutter
