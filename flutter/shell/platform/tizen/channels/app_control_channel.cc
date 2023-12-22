// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control_channel.h"

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_stream_handler_functions.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/channels/encodable_value_holder.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "tizen/internal/app_control_method";
constexpr char kEventChannelName[] = "tizen/internal/app_control_event";

}  // namespace

AppControlChannel::AppControlChannel(BinaryMessenger* messenger) {
  method_channel_ = std::make_unique<MethodChannel<EncodableValue>>(
      messenger, kChannelName, &StandardMethodCodec::GetInstance());
  method_channel_->SetMethodCallHandler(
      [this](const MethodCall<EncodableValue>& call,
             std::unique_ptr<MethodResult<EncodableValue>> result) {
        this->HandleMethodCall(call, std::move(result));
      });

  event_channel_ = std::make_unique<EventChannel<EncodableValue>>(
      messenger, kEventChannelName, &StandardMethodCodec::GetInstance());
  auto event_channel_handler = std::make_unique<StreamHandlerFunctions<>>(
      [this](const EncodableValue* arguments,
             std::unique_ptr<EventSink<>>&& events)
          -> std::unique_ptr<StreamHandlerError<>> {
        RegisterEventHandler(std::move(events));
        return nullptr;
      },
      [this](const EncodableValue* arguments)
          -> std::unique_ptr<StreamHandlerError<>> {
        UnregisterEventHandler();
        return nullptr;
      });
  event_channel_->SetStreamHandler(std::move(event_channel_handler));
}

AppControlChannel::~AppControlChannel() {}

void AppControlChannel::NotifyAppControl(void* handle) {
  auto app_control =
      std::make_unique<AppControl>(static_cast<app_control_h>(handle));
  if (!app_control->handle()) {
    FT_LOG(Error) << "Could not create an instance of AppControl.";
    return;
  }
  AppControl* app_control_ptr =
      AppControlManager::GetInstance().Insert(std::move(app_control));
  if (!app_control_ptr) {
    FT_LOG(Error) << "The handle already exists.";
    return;
  }
  if (event_sink_) {
    SendAppControlEvent(app_control_ptr);
  } else {
    FT_LOG(Info) << "No event channel has been set up.";
    queue_.push(app_control_ptr);
  }
}

void AppControlChannel::HandleMethodCall(
    const MethodCall<EncodableValue>& method_call,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  const std::string& method_name = method_call.method_name();

  const auto* arguments = std::get_if<EncodableMap>(method_call.arguments());
  if (!arguments) {
    result->Error("Invalid arguments");
    return;
  }
  EncodableValueHolder<int32_t> id(arguments, "id");
  if (!id) {
    result->Error("Invalid arguments", "No ID provided.");
    return;
  }
  AppControl* app_control = AppControlManager::GetInstance().FindById(*id);
  if (!app_control) {
    result->Error("Invalid arguments",
                  "No instance of AppControl matches the given ID.");
    return;
  }

  if (method_name == "getMatchedAppIds") {
    EncodableList app_ids;
    AppControlResult ret = app_control->GetMatchedAppIds(app_ids);
    if (ret) {
      result->Success(EncodableValue(app_ids));
    } else {
      result->Error(ret.code(), ret.message());
    }
  } else if (method_name == "reply") {
    Reply(app_control, arguments, std::move(result));
  } else if (method_name == "sendLaunchRequest") {
    SendLaunchRequest(app_control, arguments, std::move(result));
  } else if (method_name == "sendTerminateRequest") {
    SendTerminateRequest(app_control, std::move(result));
  } else if (method_name == "setAppControlData") {
    SetAppControlData(app_control, arguments, std::move(result));
  } else if (method_name == "getHandle") {
    result->Success(
        EncodableValue(reinterpret_cast<int64_t>(app_control->handle())));
  } else {
    result->NotImplemented();
  }
}

void AppControlChannel::RegisterEventHandler(
    std::unique_ptr<EventSink<EncodableValue>> events) {
  event_sink_ = std::move(events);

  // Send already queued events if any.
  while (!queue_.empty()) {
    SendAppControlEvent(queue_.front());
    queue_.pop();
  }
}

void AppControlChannel::UnregisterEventHandler() {
  event_sink_.reset();
}

void AppControlChannel::Reply(
    AppControl* app_control,
    const EncodableMap* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  EncodableValueHolder<std::string> result_str(arguments, "result");
  if (!result_str) {
    result->Error("Invalid arguments", "No result provided.");
    return;
  }

  EncodableValueHolder<int32_t> reply_id(arguments, "replyId");
  if (!reply_id) {
    result->Error("Invalid arguments", "No replyId provided.");
    return;
  }
  AppControl* reply_app_control =
      AppControlManager::GetInstance().FindById(*reply_id);
  if (!reply_app_control) {
    result->Error("Invalid arguments",
                  "No instance of AppControl matches the given ID.");
    return;
  }
  AppControlResult ret = app_control->Reply(reply_app_control, *result_str);
  if (ret) {
    result->Success();
  } else {
    result->Error(ret.code(), ret.message());
  }
}

void AppControlChannel::SendLaunchRequest(
    AppControl* app_control,
    const EncodableMap* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  EncodableValueHolder<bool> wait_for_reply(arguments, "waitForReply");
  if (wait_for_reply && *wait_for_reply) {
    auto* result_ptr = result.release();
    auto on_reply = [result_ptr](const EncodableValue& response) {
      result_ptr->Success(response);
      delete result_ptr;
    };
    AppControlResult ret = app_control->SendLaunchRequestWithReply(on_reply);
    if (!ret) {
      result_ptr->Error(ret.code(), ret.message());
      delete result_ptr;
    }
  } else {
    AppControlResult ret = app_control->SendLaunchRequest();
    if (ret) {
      result->Success();
    } else {
      result->Error(ret.code(), ret.message());
    }
  }
}

void AppControlChannel::SendTerminateRequest(
    AppControl* app_control,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  AppControlResult ret = app_control->SendTerminateRequest();
  if (ret) {
    result->Success();
  } else {
    result->Error(ret.code(), ret.message());
  }
}

void AppControlChannel::SetAppControlData(
    AppControl* app_control,
    const EncodableMap* arguments,
    std::unique_ptr<MethodResult<EncodableValue>> result) {
  EncodableValueHolder<std::string> app_id(arguments, "appId");
  EncodableValueHolder<std::string> operation(arguments, "operation");
  EncodableValueHolder<std::string> uri(arguments, "uri");
  EncodableValueHolder<std::string> mime(arguments, "mime");
  EncodableValueHolder<std::string> category(arguments, "category");
  EncodableValueHolder<std::string> launch_mode(arguments, "launchMode");
  EncodableValueHolder<EncodableMap> extra_data(arguments, "extraData");

  std::vector<AppControlResult> results;
  if (app_id) {
    results.push_back(app_control->SetAppId(*app_id));
  }
  if (operation) {
    results.push_back(app_control->SetOperation(*operation));
  }
  if (uri) {
    results.push_back(app_control->SetUri(*uri));
  }
  if (mime) {
    results.push_back(app_control->SetMime(*mime));
  }
  if (category) {
    results.push_back(app_control->SetCategory(*category));
  }
  if (launch_mode) {
    results.push_back(app_control->SetLaunchMode(*launch_mode));
  }
  if (extra_data) {
    results.push_back(app_control->SetExtraData(*extra_data));
  }
  for (AppControlResult ret : results) {
    if (!ret) {
      result->Error(ret.code(), ret.message());
      return;
    }
  }
  result->Success();
}

void AppControlChannel::SendAppControlEvent(AppControl* app_control) {
  EncodableValue map = app_control->SerializeToMap();
  if (!map.IsNull()) {
    event_sink_->Success(map);
  }
}

}  // namespace flutter
