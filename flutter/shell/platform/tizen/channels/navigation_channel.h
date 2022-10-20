// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_NAVIGATION_CHANNEL_H_
#define EMBEDDER_NAVIGATION_CHANNEL_H_

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "rapidjson/document.h"

namespace flutter {

class NavigationChannel {
 public:
  explicit NavigationChannel(BinaryMessenger* messenger);
  virtual ~NavigationChannel();

  void SetInitialRoute(const std::string& initialRoute);
  void PushRoute(const std::string& route);
  void PopRoute();

 private:
  std::unique_ptr<MethodChannel<rapidjson::Document>> channel_;
};

}  // namespace flutter

#endif  // EMBEDDER_NAVIGATION_CHANNEL_H_
