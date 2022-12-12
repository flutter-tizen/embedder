// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_PLATFORM_CHANNEL_H_
#define EMBEDDER_PLATFORM_CHANNEL_H_

#include <cbhm.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_channel.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"
#include "rapidjson/document.h"

namespace flutter {

class PlatformChannel {
 public:
  explicit PlatformChannel(BinaryMessenger* messenger, TizenViewBase* view);
  virtual ~PlatformChannel();

 private:
  using ClipboardCallback = std::function<void(const std::string& data)>;

  void HandleMethodCall(
      const MethodCall<rapidjson::Document>& call,
      std::unique_ptr<MethodResult<rapidjson::Document>> result);

  void SystemNavigatorPop();
  void PlaySystemSound(const std::string& sound_type);
  void HapticFeedbackVibrate(const std::string& feedback_type);
  void GetClipboardData(ClipboardCallback on_data);
  void SetClipboardData(const std::string& data);
  bool ClipboardHasStrings();
  void RestoreSystemUiOverlays();
  void SetEnabledSystemUiOverlays(const std::vector<std::string>& overlays);
  void SetPreferredOrientations(const std::vector<std::string>& orientations);

  std::unique_ptr<MethodChannel<rapidjson::Document>> channel_;

  // A reference to the native view managed by FlutterTizenView.
  TizenViewBase* view_ = nullptr;

  // The clipboard history manager.
  [[maybe_unused]] cbhm_h cbhm_handle_ = nullptr;

  // A container that holds clipboard data during the engine lifetime.
  //
  // Only used by profiles that do not support the Tizen clipboard API
  // (wearable and TV).
  std::string clipboard_;

  ClipboardCallback on_clipboard_data_ = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_PLATFORM_CHANNEL_H_
