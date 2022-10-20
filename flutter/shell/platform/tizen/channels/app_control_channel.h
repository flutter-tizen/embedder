// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_APP_CONTROL_CHANNEL_H_
#define EMBEDDER_APP_CONTROL_CHANNEL_H_

#include <memory>
#include <queue>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/tizen/channels/app_control.h"

namespace flutter {

class AppControlChannel {
 public:
  explicit AppControlChannel(BinaryMessenger* messenger);
  virtual ~AppControlChannel();

  void NotifyAppControl(void* app_control);

 private:
  void HandleMethodCall(const MethodCall<EncodableValue>& method_call,
                        std::unique_ptr<MethodResult<EncodableValue>> result);
  void RegisterEventHandler(std::unique_ptr<EventSink<EncodableValue>> events);
  void UnregisterEventHandler();

  void Reply(AppControl* app_control,
             const EncodableMap* arguments,
             std::unique_ptr<MethodResult<EncodableValue>> result);
  void SendLaunchRequest(AppControl* app_control,
                         const EncodableMap* arguments,
                         std::unique_ptr<MethodResult<EncodableValue>> result);
  void SendTerminateRequest(
      AppControl* app_control,
      std::unique_ptr<MethodResult<EncodableValue>> result);
  void SetAppControlData(AppControl* app_control,
                         const EncodableMap* arguments,
                         std::unique_ptr<MethodResult<EncodableValue>> result);

  void SendAppControlEvent(AppControl* app_control);

  std::unique_ptr<MethodChannel<EncodableValue>> method_channel_;
  std::unique_ptr<EventChannel<EncodableValue>> event_channel_;
  std::unique_ptr<EventSink<EncodableValue>> event_sink_;

  // We need this queue, because there is no quarantee that EventChannel on
  // Dart side will be registered before native OnAppControl event.
  std::queue<AppControl*> queue_;
};

}  // namespace flutter

#endif  // EMBEDDER_APP_CONTROL_CHANNEL_H_
