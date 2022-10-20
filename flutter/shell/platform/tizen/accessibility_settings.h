// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ACCESSIBILITY_SETTINGS_H_
#define EMBEDDER_ACCESSIBILITY_SETTINGS_H_

#include <system/system_settings.h>

namespace flutter {

class FlutterTizenEngine;

class AccessibilitySettings {
 public:
  explicit AccessibilitySettings(FlutterTizenEngine* engine);
  virtual ~AccessibilitySettings();

 private:
  static void OnHighContrastStateChanged(system_settings_key_e key,
                                         void* user_data);
  static void OnScreenReaderStateChanged(system_settings_key_e key,
                                         void* user_data);

  [[maybe_unused]] FlutterTizenEngine* engine_;
  [[maybe_unused]] bool screen_reader_enabled_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_ACCESSIBILITY_SETTINGS_H_
