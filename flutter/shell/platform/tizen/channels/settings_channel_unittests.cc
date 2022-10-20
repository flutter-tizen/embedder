// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/common/json_message_codec.h"
#include "flutter/shell/platform/tizen/channels/settings_channel.h"
#include "flutter/shell/platform/tizen/testing/test_binary_messenger.h"
#include "gtest/gtest.h"

namespace {

constexpr char kChannelName[] = "flutter/settings";
constexpr char kAlwaysUse24HourFormatKey[] = "alwaysUse24HourFormat";
constexpr char kPlatformBrightnessKey[] = "platformBrightness";
constexpr char kTextScaleFactorKey[] = "textScaleFactor";

}  // namespace

namespace flutter {
namespace testing {

TEST(SettingsChannel, SendSettingsEventOnceWhenInitialization) {
  bool always_use24_hour_format = true;
  std::string platform_brightness;
  double scale_factor = 0;
  int call_count = 0;

  TestBinaryMessenger messenger(
      [&always_use24_hour_format, &platform_brightness, &scale_factor,
       &call_count](const std::string& channel, const uint8_t* message,
                    size_t message_size, BinaryReply reply) {
        if (channel == kChannelName) {
          auto message_doc = JsonMessageCodec::GetInstance().DecodeMessage(
              message, message_size);
          always_use24_hour_format =
              (*message_doc)[kAlwaysUse24HourFormatKey].GetBool();
          scale_factor = (*message_doc)[kTextScaleFactorKey].GetDouble();
          platform_brightness =
              (*message_doc)[kPlatformBrightnessKey].GetString();
          call_count++;
        } else {
          // Should not be reached.
          EXPECT_TRUE(false);
        }
      });

  SettingsChannel setting_channel(&messenger);

  // The Linux implementation always returns false.
  EXPECT_TRUE(always_use24_hour_format == false);

  EXPECT_TRUE(platform_brightness == "light");
  EXPECT_TRUE(scale_factor == 1.0);
  EXPECT_TRUE(call_count == 1);
}

}  // namespace testing
}  // namespace flutter
