// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app_control.h"

#include "flutter/shell/platform/tizen/channels/app_control_channel.h"
#include "flutter/shell/platform/tizen/logger.h"

intptr_t NativeInitializeDartApi(void* data) {
  return Dart_InitializeApiDL(data);
}

int32_t NativeCreateAppControl(Dart_Handle handle) {
  auto app_control = std::make_unique<flutter::AppControl>();
  if (!app_control->handle()) {
    return -1;
  }
  int32_t id = app_control->id();
  Dart_NewFinalizableHandle_DL(
      handle, app_control.get(), 64,
      [](void* isolate_callback_data, void* peer) {
        auto* app_control = reinterpret_cast<flutter::AppControl*>(peer);
        flutter::AppControlManager::GetInstance().Remove(app_control->id());
      });
  flutter::AppControlManager::GetInstance().Insert(std::move(app_control));
  return id;
}

bool NativeAttachAppControl(int32_t id, Dart_Handle handle) {
  auto* app_control = flutter::AppControlManager::GetInstance().FindById(id);
  if (!app_control || !app_control->handle()) {
    return false;
  }
  Dart_NewFinalizableHandle_DL(
      handle, app_control, 64, [](void* isolate_callback_data, void* peer) {
        auto* app_control = reinterpret_cast<flutter::AppControl*>(peer);
        flutter::AppControlManager::GetInstance().Remove(app_control->id());
      });
  return true;
}

namespace flutter {

int32_t AppControl::next_id_ = 0;

AppControl::AppControl() : id_(next_id_++) {
  app_control_h handle = nullptr;
  AppControlResult ret = app_control_create(&handle);
  if (!ret) {
    FT_LOG(Error) << "Failed to create an application control handle: "
                  << ret.message();
    return;
  }
  handle_ = handle;
}

AppControl::AppControl(app_control_h handle) : id_(next_id_++) {
  app_control_h clone = nullptr;
  AppControlResult ret = app_control_clone(&clone, handle);
  if (!ret) {
    FT_LOG(Error) << "Failed to clone an application control handle: "
                  << ret.message();
    return;
  }
  handle_ = clone;
}

AppControl::~AppControl() {
  if (handle_) {
    app_control_destroy(handle_);
  }
}

AppControlResult AppControl::GetString(std::string& string,
                                       int func(app_control_h, char**)) {
  char* output;
  AppControlResult ret = func(handle_, &output);
  if (!ret) {
    return ret;
  }
  if (output) {
    string = output;
    free(output);
  } else {
    string = "";
  }
  return APP_CONTROL_ERROR_NONE;
}

AppControlResult AppControl::SetString(const std::string& string,
                                       int func(app_control_h, const char*)) {
  return func(handle_, string.c_str());
}

AppControlResult AppControl::GetAppId(std::string& app_id) {
  return GetString(app_id, app_control_get_app_id);
}

AppControlResult AppControl::SetAppId(const std::string& app_id) {
  return SetString(app_id, app_control_set_app_id);
}

AppControlResult AppControl::GetOperation(std::string& operation) {
  return GetString(operation, app_control_get_operation);
}

AppControlResult AppControl::SetOperation(const std::string& operation) {
  return SetString(operation, app_control_set_operation);
}

AppControlResult AppControl::GetUri(std::string& uri) {
  return GetString(uri, app_control_get_uri);
}

AppControlResult AppControl::SetUri(const std::string& uri) {
  return SetString(uri, app_control_set_uri);
}

AppControlResult AppControl::GetMime(std::string& mime) {
  return GetString(mime, app_control_get_mime);
}

AppControlResult AppControl::SetMime(const std::string& mime) {
  return SetString(mime, app_control_set_mime);
}

AppControlResult AppControl::GetCategory(std::string& category) {
  return GetString(category, app_control_get_category);
}

AppControlResult AppControl::SetCategory(const std::string& category) {
  return SetString(category, app_control_set_category);
}

AppControlResult AppControl::GetLaunchMode(std::string& launch_mode) {
  app_control_launch_mode_e launch_mode_e;
  AppControlResult ret = app_control_get_launch_mode(handle_, &launch_mode_e);
  if (!ret) {
    return ret;
  }
  if (launch_mode_e == APP_CONTROL_LAUNCH_MODE_GROUP) {
    launch_mode = "group";
  } else {
    launch_mode = "single";
  }
  return APP_CONTROL_ERROR_NONE;
}

AppControlResult AppControl::SetLaunchMode(const std::string& launch_mode) {
  app_control_launch_mode_e launch_mode_e;
  if (launch_mode == "group") {
    launch_mode_e = APP_CONTROL_LAUNCH_MODE_GROUP;
  } else {
    launch_mode_e = APP_CONTROL_LAUNCH_MODE_SINGLE;
  }
  return app_control_set_launch_mode(handle_, launch_mode_e);
}

bool OnAppControlExtraDataCallback(app_control_h handle,
                                   const char* key,
                                   void* user_data) {
  auto* extra_data = static_cast<EncodableMap*>(user_data);

  bool is_array = false;
  int ret = app_control_is_extra_data_array(handle, key, &is_array);
  if (ret != APP_CONTROL_ERROR_NONE) {
    FT_LOG(Error) << "app_control_is_extra_data_array() failed at key " << key;
    return false;
  }

  if (is_array) {
    char** strings = nullptr;
    int length = 0;
    ret = app_control_get_extra_data_array(handle, key, &strings, &length);
    if (ret != APP_CONTROL_ERROR_NONE) {
      FT_LOG(Error) << "app_control_get_extra_data_array() failed at key "
                    << key;
      return false;
    }
    EncodableList list;
    for (int i = 0; i < length; i++) {
      list.push_back(EncodableValue(std::string(strings[i])));
      free(strings[i]);
    }
    free(strings);
    extra_data->insert(
        {EncodableValue(std::string(key)), EncodableValue(list)});
  } else {
    char* value = nullptr;
    ret = app_control_get_extra_data(handle, key, &value);
    if (ret != APP_CONTROL_ERROR_NONE) {
      FT_LOG(Error) << "app_control_get_extra_data() failed at key " << key;
      return false;
    }
    extra_data->insert(
        {EncodableValue(std::string(key)), EncodableValue(std::string(value))});
    free(value);
  }
  return true;
}

AppControlResult AppControl::GetExtraData(EncodableMap& map) {
  EncodableMap extra_data;
  AppControlResult ret = app_control_foreach_extra_data(
      handle_, OnAppControlExtraDataCallback, &extra_data);
  if (ret) {
    map = std::move(extra_data);
  }
  return ret;
}

AppControlResult AppControl::AddExtraData(std::string key,
                                          EncodableValue value) {
  if (std::holds_alternative<EncodableList>(value)) {
    auto strings = std::vector<const char*>();
    for (const EncodableValue& value : std::get<EncodableList>(value)) {
      if (std::holds_alternative<std::string>(value)) {
        strings.push_back(std::get<std::string>(value).c_str());
      } else {
        return APP_ERROR_INVALID_PARAMETER;
      }
    }
    return app_control_add_extra_data_array(handle_, key.c_str(),
                                            strings.data(), strings.size());
  } else if (std::holds_alternative<std::string>(value)) {
    return app_control_add_extra_data(handle_, key.c_str(),
                                      std::get<std::string>(value).c_str());
  } else {
    return APP_ERROR_INVALID_PARAMETER;
  }
}

AppControlResult AppControl::SetExtraData(const EncodableMap& map) {
  for (const auto& element : map) {
    if (!std::holds_alternative<std::string>(element.first)) {
      FT_LOG(Error) << "Invalid key. Omitting.";
      continue;
    }
    const auto& key = std::get<std::string>(element.first);
    AppControlResult ret = AddExtraData(key, element.second);
    if (!ret) {
      FT_LOG(Error)
          << "The value for the key " << key
          << " must be either a string or a list of strings. Omitting.";
      continue;
    }
  }
  return AppControlResult();
}

AppControlResult AppControl::GetCaller(std::string& caller) {
  return GetString(caller, app_control_get_caller);
}

AppControlResult AppControl::IsReplyRequested(bool& value) {
  return app_control_is_reply_requested(handle_, &value);
}

EncodableValue AppControl::SerializeToMap() {
  AppControlResult results[7];
  std::string app_id, operation, uri, mime, category, launch_mode;
  EncodableMap extra_data;
  results[0] = GetAppId(app_id);
  results[1] = GetOperation(operation);
  results[2] = GetUri(uri);
  results[3] = GetMime(mime);
  results[4] = GetCategory(category);
  results[5] = GetLaunchMode(launch_mode);
  results[6] = GetExtraData(extra_data);
  for (AppControlResult result : results) {
    if (!result) {
      FT_LOG(Error) << "Failed to serialize application control data: "
                    << result.message();
      return EncodableValue();
    }
  }
  std::string caller;
  bool should_reply = false;
  GetCaller(caller);
  IsReplyRequested(should_reply);

  EncodableMap map;
  map[EncodableValue("id")] = EncodableValue(id_);
  map[EncodableValue("appId")] =
      app_id.empty() ? EncodableValue() : EncodableValue(app_id);
  map[EncodableValue("operation")] =
      operation.empty() ? EncodableValue() : EncodableValue(operation);
  map[EncodableValue("uri")] =
      uri.empty() ? EncodableValue() : EncodableValue(uri);
  map[EncodableValue("mime")] =
      mime.empty() ? EncodableValue() : EncodableValue(mime);
  map[EncodableValue("category")] =
      category.empty() ? EncodableValue() : EncodableValue(category);
  map[EncodableValue("launchMode")] = EncodableValue(launch_mode);
  map[EncodableValue("extraData")] = EncodableValue(extra_data);
  map[EncodableValue("callerAppId")] =
      caller.empty() ? EncodableValue() : EncodableValue(caller);
  map[EncodableValue("shouldReply")] = EncodableValue(should_reply);
  return EncodableValue(map);
}

AppControlResult AppControl::GetMatchedAppIds(EncodableList& list) {
  EncodableList app_ids;
  AppControlResult ret = app_control_foreach_app_matched(
      handle_,
      [](app_control_h app_control, const char* app_id,
         void* user_data) -> bool {
        auto* app_ids = static_cast<EncodableList*>(user_data);
        app_ids->push_back(EncodableValue(app_id));
        return true;
      },
      &app_ids);
  if (ret) {
    list = std::move(app_ids);
  }
  return ret;
}

AppControlResult AppControl::SendLaunchRequest() {
  return app_control_send_launch_request(handle_, nullptr, nullptr);
}

AppControlResult AppControl::SendLaunchRequestWithReply(
    ReplyCallback on_reply) {
  auto reply_callback = [](app_control_h request, app_control_h reply,
                           app_control_result_e result, void* user_data) {
    auto* app_control = static_cast<AppControl*>(user_data);
    auto reply_app_control = std::make_unique<AppControl>(reply);
    EncodableMap map;
    map[EncodableValue("reply")] = reply_app_control->SerializeToMap();
    if (result == APP_CONTROL_RESULT_APP_STARTED) {
      map[EncodableValue("result")] = EncodableValue("appStarted");
    } else if (result == APP_CONTROL_RESULT_SUCCEEDED) {
      map[EncodableValue("result")] = EncodableValue("succeeded");
    } else if (result == APP_CONTROL_RESULT_FAILED) {
      map[EncodableValue("result")] = EncodableValue("failed");
    } else if (result == APP_CONTROL_RESULT_CANCELED) {
      map[EncodableValue("result")] = EncodableValue("canceled");
    }
    AppControlManager::GetInstance().Insert(std::move(reply_app_control));
    app_control->on_reply_(EncodableValue(map));
    app_control->on_reply_ = nullptr;
  };
  on_reply_ = on_reply;
  return app_control_send_launch_request(handle_, reply_callback, this);
}

AppControlResult AppControl::SendTerminateRequest() {
  return app_control_send_terminate_request(handle_);
}

AppControlResult AppControl::Reply(AppControl* reply,
                                   const std::string& result) {
  app_control_result_e result_e;
  if (result == "appStarted") {
    result_e = APP_CONTROL_RESULT_APP_STARTED;
  } else if (result == "succeeded") {
    result_e = APP_CONTROL_RESULT_SUCCEEDED;
  } else if (result == "failed") {
    result_e = APP_CONTROL_RESULT_FAILED;
  } else if (result == "canceled") {
    result_e = APP_CONTROL_RESULT_CANCELED;
  } else {
    return APP_CONTROL_ERROR_INVALID_PARAMETER;
  }
  return app_control_reply_to_launch_request(reply->handle_, this->handle_,
                                             result_e);
}

}  // namespace flutter
