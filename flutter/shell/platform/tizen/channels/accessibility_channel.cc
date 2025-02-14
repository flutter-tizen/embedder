// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/accessibility";
constexpr char kAccessibilityDbus[] = "org.a11y.Bus";
constexpr char kAccessibilityDbusPath[] = "/org/a11y/bus";
constexpr char kAccessibilityDbusInterface[] = "org.a11y.Bus";
constexpr char kAtspiDirectReadBus[] = "org.tizen.ScreenReader";
constexpr char kAtspiDirectReadPath[] = "/org/tizen/DirectReading";
constexpr char kAtspiDirectReadInterface[] = "org.tizen.DirectReading";

}  // namespace

static void _accessibilityBusAddressGet(void* data,
                                        const Eldbus_Message* message,
                                        Eldbus_Pending* pending) {
  Eldbus_Connection** accessibility_bus =
      static_cast<Eldbus_Connection**>(data);
  const char* error_name = nullptr;
  const char* error_message = nullptr;
  const char* socket_address = nullptr;

  if (eldbus_message_error_get(message, &error_name, &error_message)) {
    FT_LOG(Error) << "Eldbus message error. (" << error_name << " : "
                  << error_message << ")";
    return;
  }

  if (!eldbus_message_arguments_get(message, "s", &socket_address) ||
      !socket_address) {
    FT_LOG(Error) << "Could not get A11Y Bus socket address.";
    return;
  }
  *accessibility_bus = eldbus_private_address_connection_get(socket_address);
}

AccessibilityChannel::AccessibilityChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMessageCodec::GetInstance())) {
  eldbus_init();

  session_bus_ = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
  bus_ = eldbus_object_get(session_bus_, kAccessibilityDbus,
                           kAccessibilityDbusPath);

  Eldbus_Message* method = eldbus_object_method_call_new(
      bus_, kAccessibilityDbusInterface, "GetAddress");
  eldbus_object_send(bus_, method, _accessibilityBusAddressGet,
                     &accessibility_bus_, 100);

  channel_->SetMessageHandler([&](const auto& message, auto reply) {
    if (std::holds_alternative<EncodableMap>(message)) {
      auto map = std::get<EncodableMap>(message);
      EncodableValueHolder<std::string> type(&map, "type");
      EncodableValueHolder<EncodableMap> data(&map, "data");

      if (type) {
        FT_LOG(Info) << "Received " << *type << " message.";
        if (*type == "announce" && data) {
          EncodableValueHolder<std::string> msg(data.value, "message");
          if (msg && accessibility_bus_) {
            Eldbus_Message* eldbus_message = eldbus_message_method_call_new(
                kAtspiDirectReadBus, kAtspiDirectReadPath,
                kAtspiDirectReadInterface, "ReadCommand");
            Eldbus_Message_Iter* iter = eldbus_message_iter_get(eldbus_message);
            eldbus_message_iter_arguments_append(iter, "sb", msg->c_str(),
                                                 true);
            eldbus_connection_send(accessibility_bus_, eldbus_message, nullptr,
                                   nullptr, -1);
          }
        }
      }
    }
    reply(EncodableValue());
  });
}

AccessibilityChannel::~AccessibilityChannel() {
  channel_->SetMessageHandler(nullptr);

  eldbus_connection_unref(accessibility_bus_);
  eldbus_connection_unref(session_bus_);
  eldbus_object_unref(bus_);

  eldbus_shutdown();
}

}  // namespace flutter
