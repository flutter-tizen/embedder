// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/channels/lifecycle_channel.h"
#include "flutter/shell/platform/tizen/channels/string_codec.h"
#include "flutter/shell/platform/tizen/testing/test_binary_messenger.h"
#include "gtest/gtest.h"

namespace {

constexpr char kChannelName[] = "flutter/lifecycle";

constexpr char kInactive[] = "AppLifecycleState.inactive";
constexpr char kResumed[] = "AppLifecycleState.resumed";
constexpr char kPaused[] = "AppLifecycleState.paused";
constexpr char kDetached[] = "AppLifecycleState.detached";
}  // namespace

namespace flutter {
namespace testing {

std::unique_ptr<TestBinaryMessenger> CreateMessenger(
    std::string& decoded_message) {
  return std::make_unique<TestBinaryMessenger>(
      [&decoded_message](const std::string& channel, const uint8_t* message,
                         size_t message_size, BinaryReply reply) {
        if (channel == kChannelName) {
          std::unique_ptr<EncodableValue> value =
              StringCodec::GetInstance().DecodeMessage(message, message_size);
          if (auto pval = std::get_if<std::string>(value.get())) {
            decoded_message = *pval;
          }
        } else {
          // Should not be reached.
          EXPECT_TRUE(false);
        }
      });
}

TEST(LifecycleChannel, AppIsInactive) {
  std::string decoded_message;
  std::unique_ptr<TestBinaryMessenger> messenger =
      CreateMessenger(decoded_message);
  LifecycleChannel lifecycle_channel(messenger.get());

  lifecycle_channel.AppIsInactive();

  EXPECT_TRUE(decoded_message == kInactive);
}

TEST(LifecycleChannel, AppIsResumed) {
  std::string decoded_message;
  std::unique_ptr<TestBinaryMessenger> messenger =
      CreateMessenger(decoded_message);
  LifecycleChannel lifecycle_channel(messenger.get());

  lifecycle_channel.AppIsResumed();

  EXPECT_TRUE(decoded_message == kResumed);
}

TEST(LifecycleChannel, AppIsPaused) {
  std::string decoded_message;
  std::unique_ptr<TestBinaryMessenger> messenger =
      CreateMessenger(decoded_message);
  LifecycleChannel lifecycle_channel(messenger.get());

  lifecycle_channel.AppIsPaused();

  EXPECT_TRUE(decoded_message == kPaused);
}

TEST(LifecycleChannel, AppIsDetached) {
  std::string decoded_message;
  std::unique_ptr<TestBinaryMessenger> messenger =
      CreateMessenger(decoded_message);
  LifecycleChannel lifecycle_channel(messenger.get());

  lifecycle_channel.AppIsDetached();

  EXPECT_TRUE(decoded_message == kDetached);
}

}  // namespace testing
}  // namespace flutter
