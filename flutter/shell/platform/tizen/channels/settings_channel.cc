// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "settings_channel.h"

#include <system/system_settings.h>

#include "flutter/shell/platform/common/json_message_codec.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/settings";

constexpr char kTextScaleFactorKey[] = "textScaleFactor";
constexpr char kAlwaysUse24HourFormatKey[] = "alwaysUse24HourFormat";
constexpr char kPlatformBrightnessKey[] = "platformBrightness";

}  // namespace

SettingsChannel::SettingsChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<BasicMessageChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMessageCodec::GetInstance())) {
  SetUpLocaleTimeFormat();
  SetUpTextScaleFactor();
  SendSettingsEvent();

  system_settings_set_changed_cb(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR,
      [](system_settings_key_e key, void* user_data) -> void {
        auto* self = static_cast<SettingsChannel*>(user_data);
        self->SetUpLocaleTimeFormat();
        self->SendSettingsEvent();
      },
      this);
  system_settings_set_changed_cb(
      SYSTEM_SETTINGS_KEY_FONT_SIZE,
      [](system_settings_key_e key, void* user_data) -> void {
        auto* self = static_cast<SettingsChannel*>(user_data);
        self->SetUpTextScaleFactor();
        self->SendSettingsEvent();
      },
      this);
}

SettingsChannel::~SettingsChannel() {
  system_settings_unset_changed_cb(
      SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR);
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_FONT_SIZE);
}

void SettingsChannel::SendSettingsEvent() {
  rapidjson::Document event(rapidjson::kObjectType);
  rapidjson::MemoryPoolAllocator<>& allocator = event.GetAllocator();

  event.AddMember(kTextScaleFactorKey, text_scale_factor_, allocator);
  event.AddMember(kAlwaysUse24HourFormatKey, locale_time_format_, allocator);
  event.AddMember(kPlatformBrightnessKey, "light", allocator);
  channel_->Send(event);
}

void SettingsChannel::SetUpLocaleTimeFormat() {
  bool value = false;
  if (system_settings_get_value_bool(
          SYSTEM_SETTINGS_KEY_LOCALE_TIMEFORMAT_24HOUR, &value) ==
      SYSTEM_SETTINGS_ERROR_NONE) {
    locale_time_format_ = value;
  }
}

void SettingsChannel::SetUpTextScaleFactor() {
  const float small = 0.8;
  const float normal = 1.0;
  const float large = 1.5;
  const float huge = 1.9;
  const float giant = 2.5;

  int value = SYSTEM_SETTINGS_FONT_SIZE_NORMAL;
  if (system_settings_get_value_int(SYSTEM_SETTINGS_KEY_FONT_SIZE, &value) ==
      SYSTEM_SETTINGS_ERROR_NONE) {
    switch (value) {
      case SYSTEM_SETTINGS_FONT_SIZE_SMALL:
        text_scale_factor_ = small;
        break;
      case SYSTEM_SETTINGS_FONT_SIZE_LARGE:
        text_scale_factor_ = large;
        break;
      case SYSTEM_SETTINGS_FONT_SIZE_HUGE:
        text_scale_factor_ = huge;
        break;
      case SYSTEM_SETTINGS_FONT_SIZE_GIANT:
        text_scale_factor_ = giant;
        break;
      default:
        text_scale_factor_ = normal;
        break;
    }
  }
}

}  // namespace flutter
