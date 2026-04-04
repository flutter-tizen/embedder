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
    : engine_(engine),
      tts_enabled_(false),
      a11y_enabled_(false),
      a11y_proxy_(nullptr),
      cancellable_(g_cancellable_new()) {
  InitializeScreenReaderMonitoring();
  InitializeA11yMonitoring();
  UpdateSemanticsState();
  InitializeHighContrastMonitoring();
}

void AccessibilitySettings::InitializeScreenReaderMonitoring() {
  int ret = system_settings_get_value_bool(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS, &tts_enabled_);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility tts.";
  }
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS,
                                 OnScreenReaderStateChanged, this);
}

void AccessibilitySettings::InitializeA11yMonitoring() {
  GError* error = nullptr;

  a11y_proxy_ =
      g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
                                    nullptr,            // GDBusInterfaceInfo
                                    "org.a11y.Bus",     // name
                                    "/org/a11y/bus",    // object path
                                    "org.a11y.Status",  // interface
                                    cancellable_,       // GCancellable
                                    &error);            // GError

  if (error) {
    FT_LOG(Error) << "Failed to create a11y proxy: " << error->message;
    g_error_free(error);
    return;
  }

  GVariant* result = g_dbus_proxy_get_cached_property(a11y_proxy_, "IsEnabled");

  if (error) {
    FT_LOG(Error) << "Failed to get initial a11y state: " << error->message;
    g_error_free(error);
  } else {
    a11y_enabled_ = g_variant_get_boolean(result);
    g_variant_unref(result);
    FT_LOG(Info) << "Initial a11y state: "
                 << (a11y_enabled_ ? "enabled" : "disabled");
  }

  g_signal_connect_data(a11y_proxy_, "g-properties-changed",
                        G_CALLBACK(OnA11yPropertiesChanged), this, nullptr,
                        G_CONNECT_AFTER);

  FT_LOG(Info) << "Connected to a11y status changes via GDBusProxy";
}

void AccessibilitySettings::InitializeHighContrastMonitoring() {
#ifdef TV_PROFILE
  int high_contrast = 0;
  int ret = system_settings_get_value_int(
      SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST, &high_contrast);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility high contrast.";
  }
  system_settings_set_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_HIGHCONTRAST,
                                 OnHighContrastStateChanged, this);

  engine_->UpdateAccessibilityFeatures(false, high_contrast);
#endif
}

void AccessibilitySettings::UpdateSemanticsState() {
  bool should_enable = tts_enabled_ || a11y_enabled_;
  if (should_enable != screen_reader_enabled_) {
    screen_reader_enabled_ = should_enable;
    engine_->SetSemanticsEnabled(should_enable);
    FT_LOG(Info) << "Semantics state updated to: "
                 << (should_enable ? "enabled" : "disabled")
                 << " (TTS: " << (tts_enabled_ ? "enabled" : "disabled")
                 << ", a11y: " << (a11y_enabled_ ? "enabled" : "disabled")
                 << ")";
  }
}

AccessibilitySettings::~AccessibilitySettings() {
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS);

  if (a11y_proxy_) {
    g_object_unref(a11y_proxy_);
    a11y_proxy_ = nullptr;
  }

  if (cancellable_) {
    g_cancellable_cancel(cancellable_);
    g_object_unref(cancellable_);
    cancellable_ = nullptr;
  }

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

  bool old_tts_enabled = self->tts_enabled_;
  int ret = system_settings_get_value_bool(key, &self->tts_enabled_);
  if (ret != SYSTEM_SETTINGS_ERROR_NONE) {
    FT_LOG(Error) << "Failed to get value of accessibility tts.";
    return;
  }

  if (old_tts_enabled != self->tts_enabled_) {
    FT_LOG(Info) << "TTS state changed to: "
                 << (self->tts_enabled_ ? "enabled" : "disabled");
    self->UpdateSemanticsState();
  }
}

void AccessibilitySettings::OnA11yPropertiesChanged(
    GDBusProxy* proxy,
    GVariant* changed_properties,
    const gchar* const* invalidated_properties,
    gpointer user_data) {
  auto* self = static_cast<AccessibilitySettings*>(user_data);

  GVariant* is_enabled = g_variant_lookup_value(changed_properties, "IsEnabled",
                                                G_VARIANT_TYPE_BOOLEAN);
  if (is_enabled) {
    bool old_a11y_enabled = self->a11y_enabled_;
    self->a11y_enabled_ = g_variant_get_boolean(is_enabled);

    if (old_a11y_enabled != self->a11y_enabled_) {
      FT_LOG(Info) << "a11y IsEnabled property changed to: "
                   << (self->a11y_enabled_ ? "enabled" : "disabled");
      self->UpdateSemanticsState();
    }
    g_variant_unref(is_enabled);
  }
}

}  // namespace flutter
