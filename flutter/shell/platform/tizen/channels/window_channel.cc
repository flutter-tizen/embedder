// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "window_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "tizen/internal/window";

}  // namespace

WindowChannel::WindowChannel(BinaryMessenger* messenger, TizenWindow* window)
    : window_(window) {
  channel_ = std::make_unique<MethodChannel<EncodableValue>>(
      messenger, kChannelName, &StandardMethodCodec::GetInstance());
  channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        this->HandleMethodCall(call, std::move(result));
      });
}

WindowChannel::~WindowChannel() {}

void WindowChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method_name = method_call.method_name();

  if (method_name == "getWindowGeometry") {
    TizenGeometry geometry = window_->GetGeometry();
    EncodableMap map;
    map[EncodableValue("x")] = EncodableValue(geometry.left);
    map[EncodableValue("y")] = EncodableValue(geometry.top);
    map[EncodableValue("width")] = EncodableValue(geometry.width);
    map[EncodableValue("height")] = EncodableValue(geometry.height);
    result->Success(EncodableValue(map));
  } else if (method_name == "setWindowGeometry") {
    const auto* arguments = std::get_if<EncodableMap>(method_call.arguments());
    if (!arguments) {
      result->Error("Invalid arguments");
      return;
    }
    EncodableValueHolder<int32_t> x(arguments, "x");
    EncodableValueHolder<int32_t> y(arguments, "y");
    EncodableValueHolder<int32_t> width(arguments, "width");
    EncodableValueHolder<int32_t> height(arguments, "height");

    TizenGeometry geometry = window_->GetGeometry();
    if (window_->SetGeometry({
            x ? *x : geometry.left,
            y ? *y : geometry.top,
            width ? *width : geometry.width,
            height ? *height : geometry.height,
        })) {
      result->Success();
    } else {
      result->NotImplemented();
    }
  } else if (method_name == "getScreenGeometry") {
    TizenGeometry geometry = window_->GetScreenGeometry();
    EncodableMap map;
    map[EncodableValue("width")] = EncodableValue(geometry.width);
    map[EncodableValue("height")] = EncodableValue(geometry.height);
    result->Success(EncodableValue(map));
  } else if (method_name == "getRotation") {
    result->Success(EncodableValue(window_->GetRotation()));
  } else {
    result->NotImplemented();
  }
}

}  // namespace flutter
