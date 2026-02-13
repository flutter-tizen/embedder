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

static void _accessibilityBusAddressGet(GObject* source_object,
                                        GAsyncResult* res,
                                        gpointer user_data) {
  g_autoptr(GError) error = nullptr;
  g_autoptr(GVariant) result = nullptr;
  GDBusConnection** accessibility_bus =
      static_cast<GDBusConnection**>(user_data);

  GDBusProxy* proxy = G_DBUS_PROXY(source_object);
  result = g_dbus_proxy_call_finish(proxy, res, &error);

  if (error) {
    FT_LOG(Error) << "GDBus message error: " << error->message;
    return;
  }

  const gchar* socket_address = nullptr;
  if (g_variant_is_of_type(result, G_VARIANT_TYPE("(s)"))) {
    g_variant_get(result, "(&s)", &socket_address);
  }

  if (!socket_address) {
    FT_LOG(Error) << "Could not get A11Y Bus socket address.";
    return;
  }

  *accessibility_bus = g_dbus_connection_new_for_address_sync(
      socket_address, G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, nullptr,
      nullptr, &error);

  if (error) {
    FT_LOG(Error) << "Failed to connect to A11Y Bus: " << error->message;
  }
}

AccessibilityChannel::AccessibilityChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMessageCodec::GetInstance())) {
  g_autoptr(GError) error = nullptr;
  session_bus_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
  if (error) {
    FT_LOG(Error) << "Failed to get session bus: " << error->message;
    return;
  }

  bus_ = g_dbus_proxy_new_sync(session_bus_, G_DBUS_PROXY_FLAGS_NONE, nullptr,
                               kAccessibilityDbus, kAccessibilityDbusPath,
                               kAccessibilityDbusInterface, nullptr, &error);

  if (error) {
    FT_LOG(Error) << "Failed to create proxy: " << error->message;
    return;
  }

  g_dbus_proxy_call(bus_, "GetAddress", nullptr, G_DBUS_CALL_FLAGS_NONE, -1,
                    nullptr, (GAsyncReadyCallback)_accessibilityBusAddressGet,
                    &accessibility_bus_);

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
            GVariant* params = g_variant_new("(sb)", msg->c_str(), TRUE);
            g_dbus_connection_call(
                accessibility_bus_, kAtspiDirectReadBus, kAtspiDirectReadPath,
                kAtspiDirectReadInterface, "ReadCommand", params, nullptr,
                G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
          }
        }
      }
    }
    reply(EncodableValue());
  });
}

AccessibilityChannel::~AccessibilityChannel() {
  channel_->SetMessageHandler(nullptr);

  if (accessibility_bus_) {
    g_object_unref(accessibility_bus_);
    accessibility_bus_ = nullptr;
  }

  if (session_bus_) {
    g_object_unref(session_bus_);
    session_bus_ = nullptr;
  }

  if (bus_) {
    g_object_unref(bus_);
    bus_ = nullptr;
  }
}

}  // namespace flutter
