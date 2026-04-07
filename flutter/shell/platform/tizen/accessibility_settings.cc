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
      dbus_is_enabled_(false),
      dbus_screen_reader_enabled_(false),
      dbus_proxy_(nullptr),
      cancellable_(g_cancellable_new()) {
  ConnectToA11yBus();
  InitializeHighContrast();
}

void AccessibilitySettings::ConnectToA11yBus() {
  g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE,
                           nullptr,            // GDBusInterfaceInfo
                           "org.a11y.Bus",     // name
                           "/org/a11y/bus",    // object path
                           "org.a11y.Status",  // interface
                           cancellable_,       // GCancellable
                           OnDBusProxyReady,   // Callback
                           this);              // GError
}

void AccessibilitySettings::OnDBusProxyReady(GObject* source_object,
                                             GAsyncResult* res,
                                             gpointer user_data) {
  GError* error = nullptr;
  auto* self = static_cast<AccessibilitySettings*>(user_data);

  self->dbus_proxy_ = g_dbus_proxy_new_for_bus_finish(res, &error);
  if (!self->dbus_proxy_) {
    FT_LOG(Error) << "Failed to create GDBusProxy";
    if (error) {
      FT_LOG(Error) << error->message;
      g_error_free(error);
    }
    return;
  }
  self->SetupDBusPropertyMonitoring();
}

void AccessibilitySettings::SetupDBusPropertyMonitoring() {
  UpdateStateFromProxy("IsEnabled", dbus_is_enabled_);
  UpdateStateFromProxy("ScreenReaderEnabled", dbus_screen_reader_enabled_);
  UpdateSemanticsState();

  g_signal_connect_data(dbus_proxy_, "g-properties-changed",
                        G_CALLBACK(OnDBusPropertiesChanged), this, nullptr,
                        G_CONNECT_AFTER);
}

void AccessibilitySettings::UpdateStateFromProxy(const char* property_name,
                                                 bool& property_state) {
  GVariant* result =
      g_dbus_proxy_get_cached_property(dbus_proxy_, property_name);

  if (result) {
    property_state = g_variant_get_boolean(result);
    g_variant_unref(result);
  } else {
    FT_LOG(Error) << "Failed to get " << property_name << " state.";
  }
}

void AccessibilitySettings::InitializeHighContrast() {
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
  bool should_enable = dbus_screen_reader_enabled_ || dbus_is_enabled_;
  if (should_enable != semantics_enabled_) {
    semantics_enabled_ = should_enable;
    engine_->SetSemanticsEnabled(should_enable);
    FT_LOG(Debug) << "Semantics state updated to: "
                  << (should_enable ? "enabled" : "disabled") << " (TTS: "
                  << (dbus_screen_reader_enabled_ ? "enabled" : "disabled")
                  << ", a11y: " << (dbus_is_enabled_ ? "enabled" : "disabled")
                  << ")";
  }
}

AccessibilitySettings::~AccessibilitySettings() {
  system_settings_unset_changed_cb(SYSTEM_SETTINGS_KEY_ACCESSIBILITY_TTS);

  if (dbus_proxy_) {
    g_object_unref(dbus_proxy_);
    dbus_proxy_ = nullptr;
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

void AccessibilitySettings::OnDBusPropertiesChanged(
    GDBusProxy* proxy,
    GVariant* changed_properties,
    const gchar* const* invalidated_properties,
    gpointer user_data) {
  auto* self = static_cast<AccessibilitySettings*>(user_data);
  bool is_changed = false;

  GVariant* is_enabled = g_variant_lookup_value(changed_properties, "IsEnabled",
                                                G_VARIANT_TYPE_BOOLEAN);
  if (is_enabled) {
    bool old_dbus_is_enabled = self->dbus_is_enabled_;
    self->dbus_is_enabled_ = g_variant_get_boolean(is_enabled);
    if (old_dbus_is_enabled != self->dbus_is_enabled_) {
      is_changed = true;
    }
    g_variant_unref(is_enabled);
  }

  GVariant* tts_enabled = g_variant_lookup_value(
      changed_properties, "ScreenReaderEnabled", G_VARIANT_TYPE_BOOLEAN);
  if (tts_enabled) {
    bool old_dbus_screen_reader_enabled = self->dbus_screen_reader_enabled_;
    self->dbus_screen_reader_enabled_ = g_variant_get_boolean(tts_enabled);
    if (old_dbus_screen_reader_enabled != self->dbus_screen_reader_enabled_) {
      is_changed = true;
    }
    g_variant_unref(tts_enabled);
  }

  if (is_changed) {
    self->UpdateSemanticsState();
  }
}

}  // namespace flutter
