// Copyright 2023 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mouse_cursor_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/mousecursor";
constexpr char kActivateSystemCursorMethod[] = "activateSystemCursor";
constexpr char kKindKey[] = "kind";

}  // namespace

MouseCursorChannel::MouseCursorChannel(BinaryMessenger* messenger,
                                       TizenViewBase* view)
    : view_(view) {
  channel_ = std::make_unique<MethodChannel<EncodableValue>>(
      messenger, kChannelName, &StandardMethodCodec::GetInstance());
  channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

MouseCursorChannel::~MouseCursorChannel() {}

void MouseCursorChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method_name = method_call.method_name();
  if (method_name == kActivateSystemCursorMethod) {
    const auto& arguments = std::get<EncodableMap>(*method_call.arguments());
    auto kind_iter = arguments.find(EncodableValue(std::string(kKindKey)));
    if (kind_iter == arguments.end()) {
      result->Error("Argument error",
                    "Missing argument while trying to activate system cursor");
      return;
    }

    const auto& kind = std::get<std::string>(kind_iter->second);
    view_->UpdateFlutterCursor(kind);
    result->Success();
  } else {
    result->NotImplemented();
  }
}

}  // namespace flutter
