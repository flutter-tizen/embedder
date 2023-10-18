// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "input_device_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "tizen/internal/inputdevice";

}  // namespace

InputDeviceChannel::InputDeviceChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<MethodChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        this->HandleMethodCall(call, std::move(result));
      });
}

InputDeviceChannel::~InputDeviceChannel() {}

void InputDeviceChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method_name = method_call.method_name();

  if (method_name == "getLastUsedKeyboard") {
    EncodableMap map;
    map[EncodableValue("name")] = EncodableValue(last_keyboard_name_);
    result->Success(EncodableValue(map));
  } else {
    result->NotImplemented();
  }
}

}  // namespace flutter
