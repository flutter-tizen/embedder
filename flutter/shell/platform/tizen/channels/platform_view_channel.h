// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_PLATFORM_VIEW_CHANNEL_H_
#define EMBEDDER_PLATFORM_VIEW_CHANNEL_H_

#include <map>
#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"

class PlatformView;
class PlatformViewFactory;

namespace flutter {

class PlatformViewChannel {
 public:
  explicit PlatformViewChannel(BinaryMessenger* messenger);
  virtual ~PlatformViewChannel();

  void Dispose();

  std::map<std::string, std::unique_ptr<PlatformViewFactory>>& ViewFactories() {
    return view_factories_;
  }

  bool SendKey(const char* key,
               const char* string,
               const char* compose,
               uint32_t modifiers,
               uint32_t scan_code,
               bool is_down);

 private:
  PlatformView* FindViewById(int view_id);
  PlatformView* FindFocusedView();

  void RemoveViewIfExists(int view_id);
  void ClearViews();
  void ClearViewFactories();

  void HandleMethodCall(const MethodCall<EncodableValue>& call,
                        std::unique_ptr<MethodResult<EncodableValue>> result);

  void OnCreate(const EncodableValue* arguments,
                std::unique_ptr<MethodResult<EncodableValue>>&& result);
  void OnClearFocus(const EncodableValue* arguments,
                    std::unique_ptr<MethodResult<EncodableValue>>&& result);
  void OnDispose(const EncodableValue* arguments,
                 std::unique_ptr<MethodResult<EncodableValue>>&& result);
  void OnResize(const EncodableValue* arguments,
                std::unique_ptr<MethodResult<EncodableValue>>&& result);
  void OnTouch(const EncodableValue* arguments,
               std::unique_ptr<MethodResult<EncodableValue>>&& result);

  std::unique_ptr<MethodChannel<EncodableValue>> channel_;
  std::map<std::string, std::unique_ptr<PlatformViewFactory>> view_factories_;
  std::map<int, PlatformView*> views_;
};

}  // namespace flutter

#endif  // EMBEDDER_PLATFORM_VIEW_CHANNEL_H_
