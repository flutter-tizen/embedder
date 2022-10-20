// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_view_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/platform_views";

}  // namespace

PlatformViewChannel::PlatformViewChannel(BinaryMessenger* messenger)
    : channel_(std::make_unique<MethodChannel<EncodableValue>>(
          messenger,
          kChannelName,
          &StandardMethodCodec::GetInstance())) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformViewChannel::~PlatformViewChannel() {
  Dispose();
}

void PlatformViewChannel::Dispose() {
  ClearViews();
  ClearViewFactories();
}

PlatformView* PlatformViewChannel::FindViewById(int view_id) {
  auto iter = views_.find(view_id);
  if (iter != views_.end()) {
    return iter->second;
  }
  return nullptr;
}

PlatformView* PlatformViewChannel::FindFocusedView() {
  for (const auto& [view_id, view] : views_) {
    if (view->IsFocused()) {
      return view;
    }
  }
  return nullptr;
}

void PlatformViewChannel::RemoveViewIfExists(int view_id) {
  PlatformView* view = FindViewById(view_id);
  if (view) {
    view->Dispose();
    delete view;
    views_.erase(view_id);
  }
}

void PlatformViewChannel::ClearViews() {
  for (const auto& [view_id, view] : views_) {
    view->Dispose();
    delete view;
  }
  views_.clear();
}

void PlatformViewChannel::ClearViewFactories() {
  for (const auto& [view_type, view_factory] : view_factories_) {
    view_factory->Dispose();
  }
  view_factories_.clear();
}

bool PlatformViewChannel::SendKey(const char* key,
                                  const char* string,
                                  const char* compose,
                                  uint32_t modifiers,
                                  uint32_t scan_code,
                                  bool is_down) {
  PlatformView* view = FindFocusedView();
  if (view) {
    return view->SendKey(key, string, compose, modifiers, scan_code, is_down);
  }
  return false;
}

void PlatformViewChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method = call.method_name();
  const EncodableValue* arguments = call.arguments();

  if (method == "create") {
    OnCreate(arguments, std::move(result));
  } else if (method == "clearFocus") {
    OnClearFocus(arguments, std::move(result));
  } else if (method == "dispose") {
    OnDispose(arguments, std::move(result));
  } else if (method == "resize") {
    OnResize(arguments, std::move(result));
  } else if (method == "touch") {
    OnTouch(arguments, std::move(result));
  } else {
    FT_LOG(Warn) << "Unimplemented method: " << method;
    result->NotImplemented();
  }
}

void PlatformViewChannel::OnCreate(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto* map = std::get_if<EncodableMap>(arguments);
  if (!map) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<std::string> view_type(map, "viewType");
  EncodableValueHolder<int> view_id(map, "id");
  EncodableValueHolder<double> width(map, "width");
  EncodableValueHolder<double> height(map, "height");

  if (!view_type || !view_id || !width || !height) {
    result->Error("Invalid arguments");
    return;
  }

  FT_LOG(Info) << "Creating a platform view: " << view_type.value;
  RemoveViewIfExists(*view_id);

  EncodableValueHolder<ByteMessage> params(map, "params");
  ByteMessage byte_message;
  if (params) {
    byte_message = *params;
  }
  auto iter = view_factories_.find(*view_type);
  if (iter != view_factories_.end()) {
    PlatformView* focused_view = FindFocusedView();
    if (focused_view) {
      focused_view->SetFocus(false);
    }
    PlatformView* view =
        iter->second->Create(*view_id, *width, *height, byte_message);
    if (view) {
      views_[*view_id] = view;
      result->Success(EncodableValue(view->GetTextureId()));
    } else {
      result->Error("Can't create view instance");
    }
  } else {
    FT_LOG(Error) << "Can't find view type: " << *view_type;
    result->Error("Can't find view type");
  }
}

void PlatformViewChannel::OnClearFocus(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  const int* view_id = std::get_if<int>(arguments);
  if (!view_id) {
    result->Error("Invalid arguments");
    return;
  }

  PlatformView* view = FindViewById(*view_id);
  if (!view) {
    result->Error("Can't find view id");
    return;
  }
  view->SetFocus(false);
  view->ClearFocus();

  result->Success();
}

void PlatformViewChannel::OnDispose(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto* map = std::get_if<EncodableMap>(arguments);
  if (!map) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<int> view_id(map, "id");
  if (!view_id) {
    result->Error("Invalid arguments");
    return;
  }

  PlatformView* view = FindViewById(*view_id);
  if (!view) {
    result->Error("Can't find view id");
    return;
  }
  RemoveViewIfExists(*view_id);

  result->Success();
}

void PlatformViewChannel::OnResize(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto* map = std::get_if<EncodableMap>(arguments);
  if (!map) {
    result->Error("Invalid arguments");
    return;
  }

  EncodableValueHolder<int> view_id(map, "id");
  EncodableValueHolder<double> width(map, "width");
  EncodableValueHolder<double> height(map, "height");

  if (!view_id || !width || !height) {
    result->Error("Invalid arguments");
    return;
  }

  PlatformView* view = FindViewById(*view_id);
  if (!view) {
    result->Error("Can't find view id");
    return;
  }
  view->Resize(*width, *height);

  result->Success(*arguments);
}

void PlatformViewChannel::OnTouch(
    const EncodableValue* arguments,
    std::unique_ptr<MethodResult<EncodableValue>>&& result) {
  auto* map = std::get_if<EncodableMap>(arguments);
  if (!map) {
    result->Error("Invalid arguments");
    return;
  }

  int type = 0, button = 0;
  double x = 0.0, y = 0.0, dx = 0.0, dy = 0.0;

  EncodableValueHolder<EncodableList> event(map, "event");
  EncodableValueHolder<int> view_id(map, "id");

  if (!view_id || !event || event->size() != 6) {
    result->Error("Invalid arguments");
    return;
  }

  type = std::get<int>(event->at(0));
  button = std::get<int>(event->at(1));
  x = std::get<double>(event->at(2));
  y = std::get<double>(event->at(3));
  dx = std::get<double>(event->at(4));
  dy = std::get<double>(event->at(5));

  PlatformView* view = FindViewById(*view_id);
  if (!view) {
    result->Error("Can't find view id");
    return;
  }
  view->Touch(type, button, x, y, dx, dy);

  if (!view->IsFocused()) {
    PlatformView* focused_view = FindFocusedView();
    if (focused_view) {
      focused_view->SetFocus(false);
    }

    view->SetFocus(true);
    if (channel_ != nullptr) {
      auto args = std::make_unique<EncodableValue>(*view_id);
      channel_->InvokeMethod("viewFocused", std::move(args));
    }
  }

  result->Success();
}

}  // namespace flutter
