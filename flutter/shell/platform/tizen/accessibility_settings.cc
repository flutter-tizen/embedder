// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_settings.h"

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

// SYSTEM_SETTINGS_KEY_MENU_SYSTEM_ACCESSIBILITY_HIGHCONTRAST = 10059 has been
// defined in system_settings_keys.h only for TV profile.
#define SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST \
  system_settings_key_e(10059)

namespace flutter {

AccessibilitySettings::AccessibilitySettings(FlutterTizenEngine* engine)
    : engine_(engine) {
  bool tts_enabled = false;
  int ret = system_settings_get_value_bool(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS, &tts_enabled);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility tts.";
  }
  if (tts_enabled) {
    screen_reader_enabled_ = tts_enabled;
    engine_->SetSemanticsEnabled(tts_enabled);
  }
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS,
                                 OnScreenReaderStateChanged, this);

#ifdef TV_PROFILE
  int high_contrast = 0;
  ret = system_settings_get_value_int(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST, &high_contrast);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility high contrast.";
  }
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST,
                                 OnHighContrastStateChanged, this);

  engine_->UpdateAccessibilityFeatures(false, high_contrast);
#endif
}

AccessibilitySettings::~AccessibilitySettings() {
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS);
#ifdef TV_PROFILE
  system_settings_unset_changed_cb(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST);
#endif
}

void AccessibilitySettings::OnHighContrastStateChanged(
    system_settings_key_e key,
    void* user_data) {
#ifdef TV_PROFILE
  auto* self = static_cast<AccessibilitySettings*>(user_data);

  int enabled = 0;
  int ret = system_settings_get_value_int(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST, &enabled);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility high contrast.";
    return;
  }

  self->engine_->UpdateAccessibilityFeatures(false, enabled);
#endif
}

void AccessibilitySettings::OnScreenReaderStateChanged(
    system_settings_key_e key,
    void* user_data) {
  auto* self = static_cast<AccessibilitySettings*>(user_data);

  bool enabled = false;
  int ret = system_settings_get_value_bool(key, &enabled);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility tts.";
    return;
  }

  if (enabled != self->screen_reader_enabled_) {
    self->screen_reader_enabled_ = enabled;
    self->engine_->SetSemanticsEnabled(enabled);
  }
}

}  // namespace flutter
