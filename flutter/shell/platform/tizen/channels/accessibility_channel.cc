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

static void _readCommandCallback(GObject* source_object,
                                 GAsyncResult* res,
                                 gpointer user_data) {
  g_autoptr(GError) error = nullptr;

  g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res, &error);

  if (error) {
    FT_LOG(Error) << "ReadCommand failed: " << error->message;
  } else {
    FT_LOG(Info) << "ReadCommand succeeded";
  }
}

void AccessibilityChannel::OnAccessibilityBusAddressGet(GObject* source_object,
                                                        GAsyncResult* res,
                                                        gpointer user_data) {
  if (!user_data) {
    FT_LOG(Error) << "Accessibility channel is not available.";
    return;
  }

  g_autoptr(GError) error = nullptr;
  g_autoptr(GVariant) result = nullptr;

  result = g_dbus_connection_call_finish(G_DBUS_CONNECTION(source_object), res,
                                         &error);
  if (error) {
    FT_LOG(Error) << "Failed to connect session bus: " << error->message;
    return;
  } else if (!result || !g_variant_is_of_type(result, G_VARIANT_TYPE("(s)"))) {
    FT_LOG(Error) << "Unexpected GetAddress response type.";
    return;
  }

  const gchar* socket_address = nullptr;
  g_variant_get(result, "(&s)", &socket_address);
  if (!socket_address || socket_address[0] == '\0') {
    FT_LOG(Error) << "Could not get A11Y Bus socket address.";
    return;
  }

  auto* self = static_cast<AccessibilityChannel*>(user_data);
  self->accessibility_bus_ = g_dbus_connection_new_for_address_sync(
      socket_address,
      static_cast<GDBusConnectionFlags>(
          G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
          G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION),
      nullptr, nullptr, &error);
  if (error) {
    FT_LOG(Error) << "Failed to connect to A11Y Bus: " << error->message;
    return;
  }

  g_dbus_connection_set_exit_on_close(self->accessibility_bus_, FALSE);

  FT_LOG(Info) << "Successfully connected to A11Y Bus at:  " << socket_address;
}

void AccessibilityChannel::OnSessionBusConnection(GObject* source_object,
                                                  GAsyncResult* res,
                                                  gpointer user_data) {
  if (!user_data) {
    FT_LOG(Error) << "Accessibility channel is not available.";
    return;
  }

  g_autoptr(GError) error = nullptr;
  GDBusConnection* session_bus = g_bus_get_finish(res, &error);
  if (error) {
    FT_LOG(Error) << "Failed to get session bus: " << error->message;
    return;
  }

  auto* self = static_cast<AccessibilityChannel*>(user_data);
  if (self->session_bus_) {
    g_object_unref(self->session_bus_);
  }
  self->session_bus_ = session_bus;

  g_dbus_connection_call(
      session_bus, kAccessibilityDbus, kAccessibilityDbusPath,
      kAccessibilityDbusInterface, "GetAddress", nullptr, G_VARIANT_TYPE("(s)"),
      G_DBUS_CALL_FLAGS_NONE, -1, self->cancellable_,
      (GAsyncReadyCallback)OnAccessibilityBusAddressGet, self);
}

AccessibilityChannel::AccessibilityChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMessageCodec::GetInstance())),
      cancellable_(g_cancellable_new()) {
  g_bus_get(G_BUS_TYPE_SESSION, cancellable_,
            (GAsyncReadyCallback)OnSessionBusConnection, this);

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
            FT_LOG(Info)
                << "A11Y Bus pointer exists, calling ReadCommand with message: "
                << msg->c_str();
            GVariant* params = g_variant_new("(sb)", msg->c_str(), TRUE);
            if (!params) {
              FT_LOG(Error) << "Failed to create GVariant parameters";
              return;
            }
            g_dbus_connection_call(
                accessibility_bus_, kAtspiDirectReadBus, kAtspiDirectReadPath,
                kAtspiDirectReadInterface, "ReadCommand", params, nullptr,
                G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
                (GAsyncReadyCallback)_readCommandCallback, nullptr);
          } else if (msg) {
            FT_LOG(Error) << "A11Y Bus is not initialized. Cannot call "
                             "ReadCommand for message: "
                          << msg->c_str();
          }
        }
      }
    }
    reply(EncodableValue());
  });
}

AccessibilityChannel::~AccessibilityChannel() {
  channel_->SetMessageHandler(nullptr);

  if (cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
    cancellable_ = nullptr;
  }

  if (accessibility_bus_) {
    g_object_unref(accessibility_bus_);
    accessibility_bus_ = nullptr;
  }

  if (session_bus_) {
    g_object_unref(session_bus_);
    session_bus_ = nullptr;
  }
}

}  // namespace flutter
