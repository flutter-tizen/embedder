// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "keyboard_channel.h"

#include <chrono>
#include <codecvt>
#include <locale>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/common/json_message_codec.h"
#include "flutter/shell/platform/tizen/channels/key_mapping.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kKeyboardChannelName[] = "flutter/keyboard";
constexpr char kKeyEventChannelName[] = "flutter/keyevent";

constexpr char kGetKeyboardStateMethod[] = "getKeyboardState";
constexpr char kKeyMapKey[] = "keymap";
constexpr char kKeyCodeKey[] = "keyCode";
constexpr char kScanCodeKey[] = "scanCode";
constexpr char kTypeKey[] = "type";
constexpr char kModifiersKey[] = "modifiers";
constexpr char kToolkitKey[] = "toolkit";
constexpr char kUnicodeScalarValuesKey[] = "unicodeScalarValues";
constexpr char kHandledKey[] = "handled";

constexpr char kKeyUp[] = "keyup";
constexpr char kKeyDown[] = "keydown";
constexpr char kGtkToolkit[] = "gtk";
constexpr char kLinuxKeyMap[] = "linux";

constexpr size_t kMaxPendingEvents = 1000;

uint32_t Utf8ToUtf32CodePoint(const char* utf8) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
#pragma clang diagnostic pop
  for (wchar_t wchar : converter.from_bytes(utf8)) {
    return wchar;
  }
  return 0;
}

uint64_t ApplyPlaneToId(uint64_t id, uint64_t plane) {
  return (id & kValueMask) | plane;
}

uint64_t GetPhysicalKey(uint32_t scan_code) {
  auto iter = kScanCodeToPhysicalKeyCode.find(scan_code);
  if (iter != kScanCodeToPhysicalKeyCode.end()) {
    return iter->second;
  }
  return ApplyPlaneToId(scan_code, kTizenPlane);
}

uint64_t GetLogicalKey(const char* key) {
  auto iter = kSymbolToLogicalKeyCode.find(key);
  if (iter != kSymbolToLogicalKeyCode.end()) {
    return iter->second;
  }
  // No logical ID is available. Default to 0.
  return ApplyPlaneToId(0, kTizenPlane);
}

uint32_t GetFallbackScanCodeFromKey(const std::string& key) {
  // Some of scan codes are 0 when key events occur from the software
  // keyboard, and key_event_channel cannot handle the key events.
  // To avoid this, use a valid scan code.

  // The following keys can be emitted from the software keyboard and have a
  // scan code with 0 value.
  const std::map<std::string, uint32_t> kKeyToScanCode = {
      {"BackSpace", 0x00000016}, {"Up", 0x0000006f},   {"Left", 0x00000071},
      {"Right", 0x00000072},     {"Down", 0x00000074},
  };

  auto iter = kKeyToScanCode.find(key);
  if (iter != kKeyToScanCode.end()) {
    return iter->second;
  }
  return 0;
}

}  // namespace

KeyboardChannel::KeyboardChannel(BinaryMessenger* messenger,
                                 SendEventHandler send_event)
    : keyboard_channel_(std::make_unique<MethodChannel<EncodableValue>>(
          messenger,
          kKeyboardChannelName,
          &StandardMethodCodec::GetInstance())),
      key_event_channel_(
          std::make_unique<BasicMessageChannel<rapidjson::Document>>(
              messenger,
              kKeyEventChannelName,
              &JsonMessageCodec::GetInstance())),
      send_event_(send_event) {
  keyboard_channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

KeyboardChannel::~KeyboardChannel() {}

void KeyboardChannel::SendKey(const char* key,
                              const char* string,
                              const char* compose,
                              uint32_t modifiers,
                              uint32_t scan_code,
                              bool is_down,
                              std::function<void(bool)> callback) {
  uint64_t sequence_id = last_sequence_id_++;

  PendingEvent pending;
  // This event is sent through the embedder API (KeyEvent) and the platform
  // channel (RawKeyEvent) simultaneously, and |callback| will be called once
  // responses are received from both of them.
  pending.unreplied = 2;
  pending.any_handled = false;
  pending.callback = std::move(callback);

  if (pending_events_.size() > kMaxPendingEvents) {
    FT_LOG(Error)
        << "There are " << pending_events_.size()
        << " keyboard events that have not yet received a response from the "
        << "framework. Are responses being sent?";
  }
  pending_events_[sequence_id] = std::make_unique<PendingEvent>(pending);

  if (scan_code == 0) {
    scan_code = GetFallbackScanCodeFromKey(key);
  }

  SendEmbedderEvent(key, string, compose, modifiers, scan_code, is_down,
                    sequence_id);
  // The channel-based API (RawKeyEvent) is deprecated and |SendChannelEvent|
  // will be removed in the future. This class (KeyboardChannel) itself will
  // also be renamed and refactored then.
  SendChannelEvent(key, string, compose, modifiers, scan_code, is_down,
                   sequence_id);
}

void KeyboardChannel::SendChannelEvent(const char* key,
                                       const char* string,
                                       const char* compose,
                                       uint32_t modifiers,
                                       uint32_t scan_code,
                                       bool is_down,
                                       uint64_t sequence_id) {
  uint32_t key_code = 0;
  auto iter = kScanCodeToGtkKeyCode.find(scan_code);
  if (iter != kScanCodeToGtkKeyCode.end()) {
    key_code = iter->second;
  }

  int gtk_modifiers = 0;
  for (auto element : kEcoreModifierToGtkModifier) {
    if (element.first & modifiers) {
      gtk_modifiers |= element.second;
    }
  }

  uint32_t unicode_scalar_values = 0;
  if (string) {
    unicode_scalar_values = Utf8ToUtf32CodePoint(string);
  }

  rapidjson::Document event(rapidjson::kObjectType);
  rapidjson::MemoryPoolAllocator<>& allocator = event.GetAllocator();
  event.AddMember(kKeyMapKey, kLinuxKeyMap, allocator);
  event.AddMember(kToolkitKey, kGtkToolkit, allocator);
  event.AddMember(kUnicodeScalarValuesKey, unicode_scalar_values, allocator);
  event.AddMember(kKeyCodeKey, key_code, allocator);
  event.AddMember(kScanCodeKey, scan_code, allocator);
  event.AddMember(kModifiersKey, gtk_modifiers, allocator);
  if (is_down) {
    event.AddMember(kTypeKey, kKeyDown, allocator);
  } else {
    event.AddMember(kTypeKey, kKeyUp, allocator);
  }
  key_event_channel_->Send(
      event, [this, sequence_id](const uint8_t* reply, size_t reply_size) {
        if (reply != nullptr) {
          std::unique_ptr<rapidjson::Document> decoded =
              JsonMessageCodec::GetInstance().DecodeMessage(reply, reply_size);
          bool handled = (*decoded)[kHandledKey].GetBool();
          ResolvePendingEvent(sequence_id, handled);
        }
      });
}

void KeyboardChannel::SendEmbedderEvent(const char* key,
                                        const char* string,
                                        const char* compose,
                                        uint32_t modifiers,
                                        uint32_t scan_code,
                                        bool is_down,
                                        uint64_t sequence_id) {
  uint64_t physical_key = GetPhysicalKey(scan_code);
  uint64_t logical_key = GetLogicalKey(key);
  const char* character = is_down ? string : nullptr;

  uint64_t last_logical_record = 0;
  auto iter = pressing_records_.find(physical_key);
  if (iter != pressing_records_.end()) {
    last_logical_record = iter->second;
  }

  FlutterKeyEventType type;
  if (is_down) {
    if (last_logical_record) {
      // A key has been pressed that has the exact physical key as a currently
      // pressed one. This can happen during repeated events.
      type = kFlutterKeyEventTypeRepeat;
    } else {
      type = kFlutterKeyEventTypeDown;
    }
    pressing_records_[physical_key] = logical_key;
  } else {
    if (!last_logical_record) {
      // The physical key has been released before. It might indicate a missed
      // event due to loss of focus, or multiple keyboards pressed keys with the
      // same physical key. Ignore the up event.
      FlutterKeyEvent empty_event = {
          .struct_size = sizeof(FlutterKeyEvent),
          .timestamp = static_cast<double>(
              std::chrono::duration_cast<std::chrono::microseconds>(
                  std::chrono::steady_clock::now().time_since_epoch())
                  .count()),
          .type = kFlutterKeyEventTypeDown,
          .physical = 0,
          .logical = 0,
          .character = "",
          .synthesized = false,
      };
      send_event_(empty_event, nullptr, nullptr);
      ResolvePendingEvent(sequence_id, true);
      return;
    } else {
      type = kFlutterKeyEventTypeUp;
    }
    pressing_records_.erase(physical_key);
  }

  FlutterKeyEvent event = {};
  event.struct_size = sizeof(FlutterKeyEvent),
  event.timestamp = static_cast<double>(
      std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
  event.type = type;
  event.physical = physical_key;
  event.logical = last_logical_record != 0 ? last_logical_record : logical_key;
  event.character = character;
  event.synthesized = false;

  send_event_(
      event,
      [](bool handled, void* user_data) {
        auto* callback = static_cast<std::function<void(bool)>*>(user_data);
        (*callback)(handled);
        delete callback;
      },
      new std::function<void(bool)>([this, sequence_id](bool handled) {
        ResolvePendingEvent(sequence_id, handled);
      }));
}

void KeyboardChannel::ResolvePendingEvent(uint64_t sequence_id, bool handled) {
  auto iter = pending_events_.find(sequence_id);
  if (iter != pending_events_.end()) {
    PendingEvent* event = iter->second.get();
    event->any_handled = event->any_handled || handled;
    event->unreplied -= 1;
    // If all handlers have replied, report if any of them handled the event.
    if (event->unreplied == 0) {
      event->callback(event->any_handled);
      pending_events_.erase(iter);
    }
    return;
  }
  // The pending event should always be found.
  FT_ASSERT_NOT_REACHED();
}

void KeyboardChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method_name = method_call.method_name();
  if (method_name == kGetKeyboardStateMethod) {
    EncodableMap map;
    for (const auto& key : pressing_records_) {
      EncodableValue physical_value(static_cast<int64_t>(key.first));
      EncodableValue logical_value(static_cast<int64_t>(key.second));
      map[physical_value] = logical_value;
    }

    result->Success(EncodableValue(map));
  } else {
    result->NotImplemented();
  }
}

}  // namespace flutter
