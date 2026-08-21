// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_window_tcore_wl.h"

#ifdef TV_PROFILE
#include <app.h>
#include <app_preference.h>
#include <time.h>
#include <vconf.h>
#include <sstream>

#include "flutter/shell/platform/tizen/tizen_window_util.h"
#endif

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_view_event_handler_delegate.h"

namespace flutter {

namespace {

constexpr int kScrollDirectionVertical = 0;
constexpr int kScrollDirectionHorizontal = 1;

#ifdef TV_PROFILE
constexpr char kSysMouseCursorPointerSizeVConfKey[] =
    "db/menu/system/mouse-pointer-size";
constexpr char kSysPointingDeviceSupportToastSharedPreferenceKey[] =
    "flutter-tizen/preference/pointing-device-support-toast";
constexpr char kTcoreWlInputCursorThemeName[] = "vd-cursors";
#endif

FlutterPointerMouseButtons ToFlutterPointerButton(int32_t button) {
  if (button == 2) {
    return kFlutterPointerButtonMouseMiddle;
  } else if (button == 3) {
    return kFlutterPointerButtonMouseSecondary;
  } else {
    return kFlutterPointerButtonMousePrimary;
  }
}

#ifdef TV_PROFILE
time_t GetBootTimeEpoch() {
  struct timespec now, boot_time;
  if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
    FT_LOG(Error) << "Fail to get clock_gettime(CLOCK_REALTIME).";
    return -1;
  }
  if (clock_gettime(CLOCK_BOOTTIME, &boot_time) != 0) {
    FT_LOG(Error) << "Fail to get clock_gettime(CLOCK_BOOTTIME).";
    return -1;
  }
  time_t boot_time_epoch = now.tv_sec - boot_time.tv_sec;
  return (boot_time_epoch / 10) * 10;
}

bool PreferenceItemCallback(const char* key, void* user_data) {
  char* app_id = (char*)user_data;
  if (!app_id || !key) {
    return true;
  }

  std::string preference_key =
      std::string(kSysPointingDeviceSupportToastSharedPreferenceKey) + "/" +
      app_id;
  if (!strncmp(key, preference_key.c_str(), preference_key.length())) {
    preference_remove(key);
  }
  return true;
}

std::string GetPreferenceKey(bool clear_exist_key) {
  time_t boot_time = GetBootTimeEpoch();
  if (boot_time == -1) {
    return "";
  }

  char* id = nullptr;
  int ret = app_get_id(&id);
  if (ret != APP_CONTROL_ERROR_NONE || !id) {
    FT_LOG(Error) << "Fail to get app id.";
    return std::string();
  }

  std::string app_id = id;
  free(id);

  std::ostringstream boot_time_buffer;
  boot_time_buffer << boot_time;

  std::string preference_key =
      std::string(kSysPointingDeviceSupportToastSharedPreferenceKey) + "/" +
      app_id + "/" + boot_time_buffer.str();

  if (clear_exist_key) {
    preference_foreach_item(PreferenceItemCallback, (void*)app_id.c_str());
  }
  return preference_key;
}

bool GetPointingDeviceToastPreference() {
  bool show_unsupported_toast = false;
  std::string preference_key = GetPreferenceKey(false);
  if (preference_key.empty()) {
    return false;
  }

  int ret =
      preference_get_boolean(preference_key.c_str(), &show_unsupported_toast);
  if (ret != PREFERENCE_ERROR_NONE) {
    return false;
  }
  return show_unsupported_toast;
}

void SetPointingDevicePreference() {
  std::string preference_key = GetPreferenceKey(true);
  if (preference_key.empty()) {
    return;
  }

  int ret = preference_set_boolean(preference_key.c_str(), true);
  if (ret != PREFERENCE_ERROR_NONE) {
    FT_LOG(Error) << "Fail to set toasted preference.";
  }
}
#endif

}  // namespace

TizenWindowTcoreWl::TizenWindowTcoreWl(TizenGeometry geometry,
                                       bool transparent,
                                       bool focusable,
                                       bool top_level,
                                       bool pointing_device_support,
                                       bool floating_menu_support,
                                       void* window_handle = nullptr,
                                       bool is_vulkan = false)
    : TizenWindow(geometry, transparent, focusable, top_level)
#ifdef TV_PROFILE
      ,
      pointing_device_support_(pointing_device_support),
      floating_menu_support_(floating_menu_support)
#endif
      ,
      is_vulkan_(is_vulkan) {
  if (!CreateWindow(window_handle)) {
    FT_LOG(Error) << "Failed to create a platform window.";
    return;
  }

  SetWindowOptions();
  RegisterEventHandlers();
  PrepareInputMethod();
  Show();
}

TizenWindowTcoreWl::~TizenWindowTcoreWl() {
  UnregisterEventHandlers();
  DestroyWindow();
}

bool TizenWindowTcoreWl::CreateWindow(void* window_handle) {
  if (tizen_core_wl_init() != TIZEN_CORE_WL_ERROR_NONE) {
    FT_LOG(Error) << "Could not initialize tizen core wl.";
    return false;
  }

  if (tizen_core_wl_display_create(&tcore_wl_display_) !=
      TIZEN_CORE_WL_ERROR_NONE) {
    FT_LOG(Error) << "Could not create tizen core wl display.";
    return false;
  }

  if (tizen_core_wl_display_connect(tcore_wl_display_, nullptr) !=
      TIZEN_CORE_WL_ERROR_NONE) {
    FT_LOG(Error) << "Tizen core wl display not found.";
    return false;
  }

  tizen_core_wl_display_private_get_wl_display(tcore_wl_display_,
                                               &wl2_display_);

  if (tizen_core_wl_display_sync(tcore_wl_display_) !=
      TIZEN_CORE_WL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to sync tizen core wl display.";
    return false;
  }

  if (tizen_core_wl_display_get_event(tcore_wl_display_, &tcore_wl_event_) !=
      TIZEN_CORE_WL_ERROR_NONE) {
    FT_LOG(Error) << "Could not get tizen core wl event handle.";
    return false;
  }

  int32_t width = 0, height = 0;
  GList* output_list = nullptr;
  tizen_core_wl_display_get_output_device_list(tcore_wl_display_, &output_list);
  if (output_list) {
    tizen_core_wl_output_h output =
        static_cast<tizen_core_wl_output_h>(output_list->data);
    tizen_core_wl_output_device_get_geometry(output, &width, &height);
    g_list_free(output_list);
  }
  if (width == 0 || height == 0) {
    FT_LOG(Error) << "Invalid screen size: " << width << " x " << height;
    return false;
  }

  if (initial_geometry_.width == 0) {
    initial_geometry_.width = width;
  }
  if (initial_geometry_.height == 0) {
    initial_geometry_.height = height;
  }

  if (window_handle == nullptr) {
    if (tizen_core_wl_create_window(
            tcore_wl_display_, nullptr, initial_geometry_.left,
            initial_geometry_.top, initial_geometry_.width,
            initial_geometry_.height,
            &tcore_wl_window_) != TIZEN_CORE_WL_ERROR_NONE) {
      FT_LOG(Error) << "Could not create tizen core wl window.";
      return false;
    }
  } else {
    tcore_wl_window_ = static_cast<tizen_core_wl_window_h>(window_handle);
  }

  if (is_vulkan_) {
    tizen_core_wl_window_private_get_wl_surface(tcore_wl_window_,
                                                &wl2_surface_);
    return wl2_surface_ && wl2_display_;
  } else {
    if (tizen_core_wl_create_egl_window(
            tcore_wl_window_, initial_geometry_.width, initial_geometry_.height,
            &tcore_wl_egl_window_) != TIZEN_CORE_WL_ERROR_NONE) {
      FT_LOG(Error) << "Could not create tizen core wl egl window.";
      return false;
    }
    return tcore_wl_egl_window_ && wl2_display_;
  }
}

void TizenWindowTcoreWl::SetWindowOptions() {
  tizen_core_wl_window_set_type(
      tcore_wl_window_, top_level_ ? TIZEN_CORE_WL_WINDOW_TYPE_NOTIFICATION
                                   : TIZEN_CORE_WL_WINDOW_TYPE_TOPLEVEL);
  if (top_level_) {
    SetNotificationLevel(TIZEN_CORE_WL_NOTIFICATION_LEVEL_TOP);
  }

  tizen_core_wl_window_set_position(tcore_wl_window_, initial_geometry_.left,
                                    initial_geometry_.top);
  tizen_core_wl_window_set_aux_hint(tcore_wl_window_,
                                    "wm.policy.win.user.geometry", "1");

  tizen_core_wl_window_set_alpha(tcore_wl_window_, transparent_);

  if (!focusable_) {
    tizen_core_wl_window_set_focus_skip(tcore_wl_window_, true);
  }

#ifdef TV_PROFILE
  tizen_core_wl_window_angle_e rotations[1] = {TIZEN_CORE_WL_WINDOW_ANGLE_0};
#else
  tizen_core_wl_window_angle_e rotations[4] = {
      TIZEN_CORE_WL_WINDOW_ANGLE_0, TIZEN_CORE_WL_WINDOW_ANGLE_90,
      TIZEN_CORE_WL_WINDOW_ANGLE_180, TIZEN_CORE_WL_WINDOW_ANGLE_270};
#endif
  tizen_core_wl_window_set_available_rotation_angle_list(
      tcore_wl_window_, rotations, sizeof(rotations) / sizeof(rotations[0]));
  EnableCursor();
}

void TizenWindowTcoreWl::EnableCursor() {
#ifdef TV_PROFILE
  TizenWindowUtil window_util;
  if (!window_util.IsValid()) {
    return;
  }

  tizen_core_wl_seat_h default_seat = nullptr;
  tizen_core_wl_display_get_default_seat(tcore_wl_display_, &default_seat);
  if (!default_seat) {
    FT_LOG(Error) << "Could not get default seat.";
    return;
  }
  struct wl_seat* seat = nullptr;
  tizen_core_wl_seat_private_get_wl_seat(default_seat, &seat);
  if (!seat) {
    FT_LOG(Error) << "Could not get wl_seat from the default seat.";
    return;
  }

  int cursor_global_id = 0;
  if (tizen_core_wl_display_private_get_global_id(
          tcore_wl_display_, "tizen_cursor", &cursor_global_id) ==
          TIZEN_CORE_WL_ERROR_NONE &&
      cursor_global_id > 0) {
    struct wl_registry* registry = wl_display_get_registry(wl2_display_);
    if (registry) {
      window_util.InitializeCursorModule(wl2_display_, registry, seat,
                                         cursor_global_id);
      wl_registry_destroy(registry);
    }
  }

  tizen_core_wl_display_sync(tcore_wl_display_);

  struct wl_surface* surface = nullptr;
  tizen_core_wl_window_private_get_wl_surface(tcore_wl_window_, &surface);
  if (surface) {
    window_util.SetCursorConfig(
        surface, TizenWindowUtil::kCursorConfigCursorAvailable, nullptr);
  }

  window_util.FinalizeCursorModule();
#endif
}

#ifdef TV_PROFILE
void TizenWindowTcoreWl::SetPointingDeviceSupport() {
  TizenWindowUtil window_util;
  window_util.SetMousePointerSupport(pointing_device_support_,
                                     tcore_wl_window_);
}

void TizenWindowTcoreWl::SetFloatingMenuSupport() {
  TizenWindowUtil window_util;
  window_util.SetMousePointerNotAllow(!floating_menu_support_,
                                      tcore_wl_window_);
}

void TizenWindowTcoreWl::ShowUnsupportedToast() {
  TizenWindowUtil window_util;
  window_util.LaunchUnsupportedToast(TizenWindowUtil::kDeviceTypeMouse, true,
                                     true, tcore_wl_window_);
}
#endif

void TizenWindowTcoreWl::AddEventListener(tizen_core_wl_event_type_e type,
                                          tizen_core_wl_event_cb callback) {
  tizen_core_wl_event_listener_h listener = nullptr;
  if (tizen_core_wl_event_add_listener(tcore_wl_event_, type, callback, this,
                                       &listener) != TIZEN_CORE_WL_ERROR_NONE) {
    FT_LOG(Error) << "Failed to add tizen core wl event listener.";
    return;
  }
  tcore_event_listeners_.push_back(listener);
}

void TizenWindowTcoreWl::RegisterEventHandlers() {
  AddEventListener(
      TIZEN_CORE_WL_EVENT_WINDOW_ROTATION,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
        if (self->view_delegate_) {
          auto* base_event =
              static_cast<tizen_core_wl_event_window_base_h>(event);
          tizen_core_wl_window_h event_window = nullptr;
          tizen_core_wl_event_window_base_get_window(base_event, &event_window);
          if (event_window == self->tcore_wl_window_) {
            tizen_core_wl_event_window_rotation_h rot_event = nullptr;
            tizen_core_wl_event_window_base_to_window_rotation(base_event,
                                                               &rot_event);
            tizen_core_wl_window_angle_e angle = TIZEN_CORE_WL_WINDOW_ANGLE_0;
            tizen_core_wl_event_window_rotation_get_angle(rot_event, &angle);
            int degree = static_cast<int>(angle);
            self->view_delegate_->OnRotate(degree);
            tizen_core_wl_window_set_rotation_angle(
                self->tcore_wl_window_,
                static_cast<tizen_core_wl_window_angle_e>(degree));
            tizen_core_wl_window_send_rotation_change_done(
                self->tcore_wl_window_, degree);
          }
        }
      });

  if (!is_vulkan_) {
    AddEventListener(
        TIZEN_CORE_WL_EVENT_WINDOW_CONFIGURE_COMPLETE,
        [](void* event, tizen_core_wl_event_type_e type, void* data) {
          auto* self = static_cast<TizenWindowTcoreWl*>(data);
          if (self->view_delegate_) {
            auto* base_event =
                static_cast<tizen_core_wl_event_window_base_h>(event);
            tizen_core_wl_window_h event_window = nullptr;
            tizen_core_wl_event_window_base_get_window(base_event,
                                                       &event_window);
            if (event_window == self->tcore_wl_window_) {
              int x = 0, y = 0, w = 0, h = 0;
              tizen_core_wl_window_get_geometry(self->tcore_wl_window_, &x, &y,
                                                &w, &h);
              int32_t rotation = self->GetRotation();
              tizen_core_wl_egl_window_resize(self->tcore_wl_egl_window_, w, h);
              tizen_core_wl_egl_window_set_window_transform(
                  self->tcore_wl_egl_window_, rotation / 90);
              self->view_delegate_->OnResize(x, y, w, h);
            }
          }
        });
  }

  AddEventListener(
      TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_DOWN,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
#ifdef TV_PROFILE
        if ((!self->pointing_device_support_ ||
             !self->floating_menu_support_) &&
            !self->show_unsupported_toast_) {
          bool shown = GetPointingDeviceToastPreference();
          if (!self->floating_menu_support_) {
            self->SetFloatingMenuSupport();
          }
          if (!shown) {
            self->ShowUnsupportedToast();
            SetPointingDevicePreference();
          }
          self->show_unsupported_toast_ = true;
          if (self->floating_menu_support_ && !self->pointing_device_support_) {
            self->SetPointingDeviceSupport();
          }
          return;
        }
#endif
        if (self->view_delegate_) {
          auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);
          tizen_core_wl_window_h event_window = nullptr;
          tizen_core_wl_event_input_base_get_window(ev, &event_window);
          if (event_window == self->tcore_wl_window_) {
            int x = 0, y = 0;
            tizen_core_wl_event_mouse_button_get_position(ev, &x, &y);
            unsigned int buttons = 0;
            tizen_core_wl_event_mouse_button_get_buttons(ev, &buttons);
            uint32_t timestamp = 0;
            tizen_core_wl_event_input_base_get_timestamp(ev, &timestamp);
            unsigned int touch_id = 0;
            tizen_core_wl_event_mouse_button_get_touch_id(ev, &touch_id);
            self->view_delegate_->OnPointerDown(
                x, y, ToFlutterPointerButton(buttons), timestamp,
                touch_id == 0 ? kFlutterPointerDeviceKindMouse
                              : kFlutterPointerDeviceKindTouch,
                touch_id);
          }
        }
      });

  AddEventListener(
      TIZEN_CORE_WL_EVENT_MOUSE_BUTTON_UP,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
        if (self->view_delegate_) {
          auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);
          tizen_core_wl_window_h event_window = nullptr;
          tizen_core_wl_event_input_base_get_window(ev, &event_window);
          if (event_window == self->tcore_wl_window_) {
            int x = 0, y = 0;
            tizen_core_wl_event_mouse_button_get_position(ev, &x, &y);
            unsigned int buttons = 0;
            tizen_core_wl_event_mouse_button_get_buttons(ev, &buttons);
            uint32_t timestamp = 0;
            tizen_core_wl_event_input_base_get_timestamp(ev, &timestamp);
            unsigned int touch_id = 0;
            tizen_core_wl_event_mouse_button_get_touch_id(ev, &touch_id);
            self->view_delegate_->OnPointerUp(
                x, y, ToFlutterPointerButton(buttons), timestamp,
                touch_id == 0 ? kFlutterPointerDeviceKindMouse
                              : kFlutterPointerDeviceKindTouch,
                touch_id);
          }
        }
      });

  AddEventListener(
      TIZEN_CORE_WL_EVENT_MOUSE_MOVE,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
        if (self->view_delegate_) {
          auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);
          tizen_core_wl_window_h event_window = nullptr;
          tizen_core_wl_event_input_base_get_window(ev, &event_window);
          if (event_window == self->tcore_wl_window_) {
            int x = 0, y = 0;
            tizen_core_wl_event_mouse_move_get_position(ev, &x, &y);
            uint32_t timestamp = 0;
            tizen_core_wl_event_input_base_get_timestamp(ev, &timestamp);
            unsigned int touch_id = 0;
            tizen_core_wl_event_mouse_move_get_touch_id(ev, &touch_id);
            self->view_delegate_->OnPointerMove(
                x, y, timestamp,
                touch_id == 0 ? kFlutterPointerDeviceKindMouse
                              : kFlutterPointerDeviceKindTouch,
                touch_id);
          }
        }
      });

  AddEventListener(
      TIZEN_CORE_WL_EVENT_MOUSE_WHEEL,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
        if (self->view_delegate_) {
          auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);
          tizen_core_wl_window_h event_window = nullptr;
          tizen_core_wl_event_input_base_get_window(ev, &event_window);
          if (event_window == self->tcore_wl_window_) {
            int x = 0, y = 0;
            tizen_core_wl_event_mouse_wheel_get_position(ev, &x, &y);
            int direction = 0;
            tizen_core_wl_event_mouse_wheel_get_direction(ev, &direction);
            int z = 0;
            tizen_core_wl_event_mouse_wheel_get_z(ev, &z);
            uint32_t timestamp = 0;
            tizen_core_wl_event_input_base_get_timestamp(ev, &timestamp);

            double delta_x = 0.0;
            double delta_y = 0.0;
            if (direction == kScrollDirectionVertical) {
              delta_y += z;
            } else if (direction == kScrollDirectionHorizontal) {
              delta_x += z;
            }

            self->view_delegate_->OnScroll(x, y, delta_x, delta_y, timestamp,
                                           kFlutterPointerDeviceKindMouse, 0);
          }
        }
      });

  AddEventListener(
      TIZEN_CORE_WL_EVENT_KEY_DOWN,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
        if (!self->view_delegate_) {
          return;
        }
        auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);
        tizen_core_wl_window_h event_window = nullptr;
        tizen_core_wl_event_input_base_get_window(ev, &event_window);
        if (event_window != self->tcore_wl_window_) {
          return;
        }

        char* keyname = nullptr;
        tizen_core_wl_event_key_get_keyname(ev, &keyname);
        char* keysymbol = nullptr;
        tizen_core_wl_event_key_get_keysymbol(ev, &keysymbol);
        char* compose = nullptr;
        tizen_core_wl_event_key_get_compose(ev, &compose);
        unsigned int modifiers = 0;
        tizen_core_wl_event_key_get_modifiers(ev, &modifiers);
        unsigned int keycode = 0;
        tizen_core_wl_event_key_get_keycode(ev, &keycode);
        char* dev_identifier = nullptr;
        tizen_core_wl_event_input_base_get_device_identifier(ev,
                                                             &dev_identifier);

        bool handled = false;
        if (self->input_method_context_ &&
            self->input_method_context_->ShouldFilterKey(keysymbol)) {
          handled =
              self->input_method_context_->HandleTcoreWlEventKey(event, true);
        }
        if (!handled) {
          // Match TizenWindowEcoreWl2 OnKey arg semantics:
          //   param 1 (key)    = XKB key symbol name (e.g. "period")
          //   param 2 (string) = composed printable string (e.g. ".")
          // tcore's keysymbol corresponds to ecore's key_event->key, and
          // tcore's compose corresponds to ecore's key_event->string.
          // Passing keysymbol as `string` breaks the printable-key fallback
          // in TextInputChannel::HandleKey() for keys whose symbol name is
          // multi-char ("period", "minus", "equal", ...).
          self->view_delegate_->OnKey(keysymbol, compose, compose, modifiers,
                                      keycode, dev_identifier, true);
        }
        free(keyname);
        free(keysymbol);
        free(compose);
        free(dev_identifier);
      });

  AddEventListener(
      TIZEN_CORE_WL_EVENT_KEY_UP,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenWindowTcoreWl*>(data);
        if (!self->view_delegate_) {
          return;
        }
        auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);
        tizen_core_wl_window_h event_window = nullptr;
        tizen_core_wl_event_input_base_get_window(ev, &event_window);
        if (event_window != self->tcore_wl_window_) {
          return;
        }

        char* keyname = nullptr;
        tizen_core_wl_event_key_get_keyname(ev, &keyname);
        char* keysymbol = nullptr;
        tizen_core_wl_event_key_get_keysymbol(ev, &keysymbol);
        char* compose = nullptr;
        tizen_core_wl_event_key_get_compose(ev, &compose);
        unsigned int modifiers = 0;
        tizen_core_wl_event_key_get_modifiers(ev, &modifiers);
        unsigned int keycode = 0;
        tizen_core_wl_event_key_get_keycode(ev, &keycode);

        bool handled = false;
        if (self->input_method_context_ &&
            self->input_method_context_->ShouldFilterKey(keysymbol)) {
          handled =
              self->input_method_context_->HandleTcoreWlEventKey(event, false);
        }
        if (!handled) {
          // See key-down handler for the keysymbol vs compose rationale.
          self->view_delegate_->OnKey(keysymbol, compose, compose, modifiers,
                                      keycode, nullptr, false);
        }
        free(keyname);
        free(keysymbol);
        free(compose);
      });
}

void TizenWindowTcoreWl::UnregisterEventHandlers() {
  for (tizen_core_wl_event_listener_h listener : tcore_event_listeners_) {
    tizen_core_wl_event_remove_listener(tcore_wl_event_, listener);
  }
  tcore_event_listeners_.clear();
}

void TizenWindowTcoreWl::DestroyWindow() {
  if (tcore_wl_egl_window_) {
    tizen_core_wl_egl_window_destroy(tcore_wl_egl_window_);
    tcore_wl_egl_window_ = nullptr;
  }

  if (tcore_wl_window_) {
    tizen_core_wl_window_destroy(tcore_wl_window_);
    tcore_wl_window_ = nullptr;
  }

  if (tcore_wl_display_) {
    tizen_core_wl_display_disconnect(tcore_wl_display_);
    tizen_core_wl_display_destroy(tcore_wl_display_);
    tcore_wl_display_ = nullptr;
  }
  tizen_core_wl_shutdown();
}

TizenGeometry TizenWindowTcoreWl::GetGeometry() {
  TizenGeometry result;
  tizen_core_wl_window_get_geometry(tcore_wl_window_, &result.left, &result.top,
                                    &result.width, &result.height);
  return result;
}

bool TizenWindowTcoreWl::SetGeometry(TizenGeometry geometry) {
  tizen_core_wl_window_set_geometry(tcore_wl_window_, geometry.left,
                                    geometry.top, geometry.width,
                                    geometry.height);
  tizen_core_wl_window_set_position(tcore_wl_window_, geometry.left,
                                    geometry.top);
  return true;
}

TizenGeometry TizenWindowTcoreWl::GetScreenGeometry() {
  TizenGeometry result = {};
  GList* output_list = nullptr;
  tizen_core_wl_display_get_output_device_list(tcore_wl_display_, &output_list);
  if (output_list) {
    tizen_core_wl_output_h output =
        static_cast<tizen_core_wl_output_h>(output_list->data);
    tizen_core_wl_output_device_get_geometry(output, &result.width,
                                             &result.height);
    g_list_free(output_list);
  }
  return result;
}

int32_t TizenWindowTcoreWl::GetRotation() {
  tizen_core_wl_window_angle_e angle = TIZEN_CORE_WL_WINDOW_ANGLE_0;
  tizen_core_wl_window_get_rotation_angle(tcore_wl_window_, &angle);
  return static_cast<int32_t>(angle);
}

int32_t TizenWindowTcoreWl::GetDpi() {
  GList* output_list = nullptr;
  tizen_core_wl_display_get_output_device_list(tcore_wl_display_, &output_list);
  if (!output_list) {
    FT_LOG(Error) << "Could not find an output device.";
    return 0;
  }
  tizen_core_wl_output_h output =
      static_cast<tizen_core_wl_output_h>(output_list->data);
  g_list_free(output_list);
  int dpi = 0;
  tizen_core_wl_output_device_get_dpi(output, &dpi);
  return dpi;
}

uintptr_t TizenWindowTcoreWl::GetWindowId() {
  return reinterpret_cast<uintptr_t>(tcore_wl_window_);
}

uint32_t TizenWindowTcoreWl::GetResourceId() {
  if (resource_id_ > 0) {
    return resource_id_;
  }

  unsigned int res_id = 0;
  if (tizen_core_wl_window_private_get_resource_id(tcore_wl_window_, &res_id) ==
      TIZEN_CORE_WL_ERROR_NONE) {
    resource_id_ = res_id;
  }
  return resource_id_;
}

void TizenWindowTcoreWl::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  std::vector<tizen_core_wl_window_angle_e> angles;
  for (int rot : rotations) {
    angles.push_back(static_cast<tizen_core_wl_window_angle_e>(rot));
  }
  tizen_core_wl_window_set_available_rotation_angle_list(
      tcore_wl_window_, angles.data(), angles.size());
}

void TizenWindowTcoreWl::BindKeys(const std::vector<std::string>& keys) {
  GList* list = nullptr;
  for (const std::string& key : keys) {
    tizen_core_wl_keygrab_info_h info = nullptr;
    tizen_core_wl_keygrab_info_create(key.c_str(),
                                      TIZEN_CORE_WL_KEYGRAB_TOPMOST, &info);
    if (info) {
      list = g_list_append(list, info);
    }
  }
  if (!list) {
    return;
  }

  tizen_core_wl_window_set_keygrab_list(tcore_wl_window_, list);

  for (GList* node = list; node; node = node->next) {
    tizen_core_wl_keygrab_info_destroy(
        static_cast<tizen_core_wl_keygrab_info_h>(node->data));
  }
  g_list_free(list);
}

void TizenWindowTcoreWl::Show() {
  tizen_core_wl_window_show(tcore_wl_window_);
}

void TizenWindowTcoreWl::UpdateFlutterCursor(const std::string& kind) {
#ifdef TV_PROFILE
  int pointer_size = -1;
  if (vconf_get_int(kSysMouseCursorPointerSizeVConfKey, &pointer_size) < 0) {
    FT_LOG(Info) << "Failed to load cursor size.";
  }

  std::string cursor_name = "normal_default";
  if (kind == "basic") {
    if (pointer_size == 0) {
      cursor_name = "large_normal";
    } else if (pointer_size == 1) {
      cursor_name = "medium_normal";
    } else if (pointer_size == 2) {
      cursor_name = "small_normal";
    } else {
      cursor_name = "normal_default";
    }
  } else if (kind == "click") {
    if (pointer_size == 0) {
      cursor_name = "large_normal_pnh";
    } else if (pointer_size == 1) {
      cursor_name = "medium_normal_pnh";
    } else if (pointer_size == 2) {
      cursor_name = "small_normal_pnh";
    } else {
      cursor_name = "normal_pnh";
    }
  } else if (kind == "text") {
    if (pointer_size == 0) {
      cursor_name = "large_normal_input_field";
    } else if (pointer_size == 1) {
      cursor_name = "medium_normal_input_field";
    } else if (pointer_size == 2) {
      cursor_name = "small_normal_input_field";
    } else {
      cursor_name = "normal_input_field";
    }
  } else if (kind == "none") {
    cursor_name = "normal_transparent";
  } else {
    FT_LOG(Info) << kind << " cursor is not supported.";
  }
  tizen_core_wl_seat_h default_seat = nullptr;
  tizen_core_wl_display_get_default_seat(tcore_wl_display_, &default_seat);
  if (default_seat) {
    tizen_core_wl_seat_set_cursor_theme(default_seat,
                                        kTcoreWlInputCursorThemeName);
    tizen_core_wl_seat_set_cursor_name(default_seat, cursor_name.c_str());
  } else {
    FT_LOG(Error) << "Failed to get default seat; cannot update cursor.";
  }
#else
  tizen_core_wl_seat_h default_seat = nullptr;
  tizen_core_wl_display_get_default_seat(tcore_wl_display_, &default_seat);
  if (default_seat) {
    tizen_core_wl_seat_set_cursor_theme(default_seat, "default");
    tizen_core_wl_seat_set_cursor_name(default_seat, "left_ptr");
  }
  FT_LOG(Info) << "UpdateFlutterCursor is not supported.";
#endif
}

void TizenWindowTcoreWl::SetNotificationLevel(int level) {
  tizen_core_wl_notification_set_level(
      tcore_wl_window_, static_cast<tizen_core_wl_notification_level_e>(level));
}

void TizenWindowTcoreWl::PrepareInputMethod() {
  input_method_context_ =
      std::make_unique<TizenInputMethodContext>(GetWindowId());

  input_method_context_->SetOnPreeditStart(
      [this]() { view_delegate_->OnComposeBegin(); });
  input_method_context_->SetOnPreeditChanged(
      [this](std::string str, int cursor_pos) {
        view_delegate_->OnComposeChange(str, cursor_pos);
      });
  input_method_context_->SetOnPreeditEnd(
      [this]() { view_delegate_->OnComposeEnd(); });
  input_method_context_->SetOnCommit(
      [this](std::string str) { view_delegate_->OnCommit(str); });
}

void* TizenWindowTcoreWl::GetRenderTarget() {
  if (is_vulkan_) {
    return wl2_surface_;
  } else {
    return tcore_wl_egl_window_;
  }
}

void TizenWindowTcoreWl::ActivateWindow() {
  tizen_core_wl_window_activate(tcore_wl_window_);
}

void TizenWindowTcoreWl::RaiseWindow() {
  tizen_core_wl_window_raise(tcore_wl_window_);
}

void TizenWindowTcoreWl::LowerWindow() {
  tizen_core_wl_window_lower(tcore_wl_window_);
}

}  // namespace flutter
