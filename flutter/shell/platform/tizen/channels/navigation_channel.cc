// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "navigation_channel.h"

#include "flutter/shell/platform/common/json_method_codec.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/navigation";

constexpr char kSetInitialRouteMethod[] = "setInitialRoute";
constexpr char kPushRouteMethod[] = "pushRoute";
constexpr char kPopRouteMethod[] = "popRoute";

}  // namespace

NavigationChannel::NavigationChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())) {}

NavigationChannel::~NavigationChannel() {}

void NavigationChannel::SetInitialRoute(const std::string& initialRoute) {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kObjectType);
  args->Parse("\"" + initialRoute + "\"");

  if (!args->HasParseError()) {
    channel_->InvokeMethod(kSetInitialRouteMethod, std::move(args));
  }
}

void NavigationChannel::PushRoute(const std::string& route) {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kObjectType);
  args->Parse("\"" + route + "\"");

  if (!args->HasParseError()) {
    channel_->InvokeMethod(kPushRouteMethod, std::move(args));
  }
}

void NavigationChannel::PopRoute() {
  channel_->InvokeMethod(kPopRouteMethod, nullptr);
}

}  // namespace flutter
