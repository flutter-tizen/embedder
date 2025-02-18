// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_window_ecore_wl2.h"

#ifdef TV_PROFILE
#include <dlfcn.h>
#include <vconf.h>
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
constexpr char kEcoreWL2InputCursorThemeName[] = "vd-cursors";
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

FlutterPointerDeviceKind ToFlutterDeviceKind(const Ecore_Device* dev) {
  Ecore_Device_Class device_class = ecore_device_class_get(dev);
  if (device_class == ECORE_DEVICE_CLASS_MOUSE) {
    return kFlutterPointerDeviceKindMouse;
  } else if (device_class == ECORE_DEVICE_CLASS_PEN) {
    return kFlutterPointerDeviceKindStylus;
  } else {
    return kFlutterPointerDeviceKindTouch;
  }
}

}  // namespace

TizenWindowEcoreWl2::TizenWindowEcoreWl2(TizenGeometry geometry,
                                         bool transparent,
                                         bool focusable,
                                         bool top_level)
    : TizenWindow(geometry, transparent, focusable, top_level) {
  if (!CreateWindow()) {
    FT_LOG(Error) << "Failed to create a platform window.";
    return;
  }

  SetWindowOptions();
  RegisterEventHandlers();
  PrepareInputMethod();
  Show();
}

TizenWindowEcoreWl2::~TizenWindowEcoreWl2() {
  UnregisterEventHandlers();
  DestroyWindow();
}

bool TizenWindowEcoreWl2::CreateWindow() {
  if (!ecore_wl2_init()) {
    FT_LOG(Error) << "Could not initialize Ecore Wl2.";
    return false;
  }

  ecore_wl2_display_ = ecore_wl2_display_connect(nullptr);
  if (!ecore_wl2_display_) {
    FT_LOG(Error) << "Ecore Wl2 display not found.";
    return false;
  }
  wl2_display_ = ecore_wl2_display_get(ecore_wl2_display_);

  ecore_wl2_sync();

  int32_t width, height;
  ecore_wl2_display_screen_size_get(ecore_wl2_display_, &width, &height);
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

  ecore_wl2_window_ = ecore_wl2_window_new(
      ecore_wl2_display_, nullptr, initial_geometry_.left,
      initial_geometry_.top, initial_geometry_.width, initial_geometry_.height);

  ecore_wl2_egl_window_ = ecore_wl2_egl_window_create(
      ecore_wl2_window_, initial_geometry_.width, initial_geometry_.height);
  return ecore_wl2_egl_window_ && wl2_display_;
}

void TizenWindowEcoreWl2::SetWindowOptions() {
  // Change the window type to use the tizen policy for notification window
  // according to top_level_.
  // Note: ECORE_WL2_WINDOW_TYPE_TOPLEVEL is similar to "ELM_WIN_BASIC" and it
  // does not mean that the window always will be overlaid on other apps :(
  ecore_wl2_window_type_set(ecore_wl2_window_,
                            top_level_ ? ECORE_WL2_WINDOW_TYPE_NOTIFICATION
                                       : ECORE_WL2_WINDOW_TYPE_TOPLEVEL);
  if (top_level_) {
    SetTizenPolicyNotificationLevel(TIZEN_POLICY_LEVEL_TOP);
  }

  ecore_wl2_window_position_set(ecore_wl2_window_, initial_geometry_.left,
                                initial_geometry_.top);
  ecore_wl2_window_aux_hint_add(ecore_wl2_window_, 0,
                                "wm.policy.win.user.geometry", "1");

  if (transparent_) {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_TRUE);
  } else {
    ecore_wl2_window_alpha_set(ecore_wl2_window_, EINA_FALSE);
  }

  if (!focusable_) {
    ecore_wl2_window_focus_skip_set(ecore_wl2_window_, EINA_TRUE);
  }

  ecore_wl2_window_indicator_state_set(ecore_wl2_window_,
                                       ECORE_WL2_INDICATOR_STATE_ON);
  ecore_wl2_window_indicator_opacity_set(ecore_wl2_window_,
                                         ECORE_WL2_INDICATOR_OPAQUE);
  ecore_wl2_indicator_visible_type_set(ecore_wl2_window_,
                                       ECORE_WL2_INDICATOR_VISIBLE_TYPE_SHOWN);

#ifdef TV_PROFILE
  int rotations[1] = {0};  // Default is only landscape.
#else
  int rotations[4] = {0, 90, 180, 270};
#endif
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations,
                                           sizeof(rotations) / sizeof(int));

  EnableCursor();
}

void TizenWindowEcoreWl2::EnableCursor() {
#ifdef TV_PROFILE
  // dlopen is used here because the TV-specific library libvd-win-util.so
  // and the relevant headers are not present in the rootstrap.
  void* handle = dlopen("libvd-win-util.so", RTLD_LAZY);
  if (!handle) {
    FT_LOG(Error) << "Could not open a shared library libvd-win-util.so.";
    return;
  }

  // These functions are defined in vd-win-util's cursor_module.h.
  int (*CursorModule_Initialize)(wl_display* display, wl_registry* registry,
                                 wl_seat* seat, unsigned int id);
  int (*Cursor_Set_Config)(wl_surface* surface, uint32_t config_type,
                           void* data);
  void (*CursorModule_Finalize)(void);
  *(void**)(&CursorModule_Initialize) =
      dlsym(handle, "CursorModule_Initialize");
  *(void**)(&Cursor_Set_Config) = dlsym(handle, "Cursor_Set_Config");
  *(void**)(&CursorModule_Finalize) = dlsym(handle, "CursorModule_Finalize");

  if (!CursorModule_Initialize || !Cursor_Set_Config ||
      !CursorModule_Finalize) {
    FT_LOG(Error) << "Could not load symbols from the library.";
    dlclose(handle);
    return;
  }

  wl_registry* registry = ecore_wl2_display_registry_get(ecore_wl2_display_);
  wl_seat* seat = ecore_wl2_input_seat_get(
      ecore_wl2_input_default_input_get(ecore_wl2_display_));
  if (!registry || !seat) {
    FT_LOG(Error)
        << "Could not retreive wl_registry or wl_seat from the display.";
    dlclose(handle);
    return;
  }

  Eina_Iterator* iter = ecore_wl2_display_globals_get(ecore_wl2_display_);
  Ecore_Wl2_Global* global = nullptr;

  EINA_ITERATOR_FOREACH(iter, global) {
    if (strcmp(global->interface, "tizen_cursor") == 0) {
      if (!CursorModule_Initialize(wl2_display_, registry, seat, global->id)) {
        FT_LOG(Error) << "Failed to initialize the cursor module.";
      }
    }
  }
  eina_iterator_free(iter);

  ecore_wl2_sync();

  wl_surface* surface = ecore_wl2_window_surface_get(ecore_wl2_window_);
  // The config_type 1 refers to TIZEN_CURSOR_CONFIG_CURSOR_AVAILABLE
  // defined in the TV extension protocol tizen-extension-tv.xml.
  if (!Cursor_Set_Config(surface, 1, nullptr)) {
    FT_LOG(Error) << "Failed to set a cursor config value.";
  }

  CursorModule_Finalize();
  dlclose(handle);
#endif
}

void TizenWindowEcoreWl2::RegisterEventHandlers() {
  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_WL2_EVENT_WINDOW_ROTATE,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* rotation_event =
              reinterpret_cast<Ecore_Wl2_Event_Window_Rotation*>(event);
          if (rotation_event->win == self->GetWindowId()) {
            int32_t degree = rotation_event->angle;
            self->view_delegate_->OnRotate(degree);
            TizenGeometry geometry = self->GetGeometry();
            ecore_wl2_window_rotation_set(self->ecore_wl2_window_, degree);
            ecore_wl2_window_rotation_change_done_send(
                self->ecore_wl2_window_, rotation_event->rotation,
                geometry.width, geometry.height);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_WL2_EVENT_WINDOW_CONFIGURE,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* configure_event =
              reinterpret_cast<Ecore_Wl2_Event_Window_Configure*>(event);
          if (configure_event->win == self->GetWindowId()) {
            ecore_wl2_egl_window_resize_with_rotation(
                self->ecore_wl2_egl_window_, configure_event->x,
                configure_event->y, configure_event->w, configure_event->h,
                self->GetRotation());

            self->view_delegate_->OnResize(
                configure_event->x, configure_event->y, configure_event->w,
                configure_event->h);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_BUTTON_DOWN,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* button_event =
              reinterpret_cast<Ecore_Event_Mouse_Button*>(event);
          if (button_event->window == self->GetWindowId()) {
            self->view_delegate_->OnPointerDown(
                button_event->x, button_event->y,
                ToFlutterPointerButton(button_event->buttons),
                button_event->timestamp, ToFlutterDeviceKind(button_event->dev),
                button_event->multi.device);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_BUTTON_UP,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* button_event =
              reinterpret_cast<Ecore_Event_Mouse_Button*>(event);
          if (button_event->window == self->GetWindowId()) {
            self->view_delegate_->OnPointerUp(
                button_event->x, button_event->y,
                ToFlutterPointerButton(button_event->buttons),
                button_event->timestamp, ToFlutterDeviceKind(button_event->dev),
                button_event->multi.device);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_MOVE,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* move_event = reinterpret_cast<Ecore_Event_Mouse_Move*>(event);
          if (move_event->window == self->GetWindowId()) {
            self->view_delegate_->OnPointerMove(
                move_event->x, move_event->y, move_event->timestamp,
                ToFlutterDeviceKind(move_event->dev), move_event->multi.device);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_MOUSE_WHEEL,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* wheel_event = reinterpret_cast<Ecore_Event_Mouse_Wheel*>(event);
          if (wheel_event->window == self->GetWindowId()) {
            double delta_x = 0.0;
            double delta_y = 0.0;

            if (wheel_event->direction == kScrollDirectionVertical) {
              delta_y += wheel_event->z;
            } else if (wheel_event->direction == kScrollDirectionHorizontal) {
              delta_x += wheel_event->z;
            }

            self->view_delegate_->OnScroll(
                wheel_event->x, wheel_event->y, delta_x, delta_y,
                wheel_event->timestamp, ToFlutterDeviceKind(wheel_event->dev),
                0);
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_KEY_DOWN,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* key_event = reinterpret_cast<Ecore_Event_Key*>(event);
          if (key_event->window == self->GetWindowId()) {
            bool handled = false;
            if (self->input_method_context_->IsInputPanelShown()) {
              handled = self->input_method_context_->HandleEcoreEventKey(
                  key_event, true);
            }
            if (!handled) {
              self->view_delegate_->OnKey(
                  key_event->key, key_event->string, key_event->compose,
                  key_event->modifiers, key_event->keycode,
                  ecore_device_name_get(key_event->dev), true);
            }
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));

  ecore_event_handlers_.push_back(ecore_event_handler_add(
      ECORE_EVENT_KEY_UP,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = static_cast<TizenWindowEcoreWl2*>(data);
        if (self->view_delegate_) {
          auto* key_event = reinterpret_cast<Ecore_Event_Key*>(event);
          if (key_event->window == self->GetWindowId()) {
            bool handled = false;
            if (self->input_method_context_->IsInputPanelShown()) {
              handled = self->input_method_context_->HandleEcoreEventKey(
                  key_event, false);
            }
            if (!handled) {
              self->view_delegate_->OnKey(
                  key_event->key, key_event->string, key_event->compose,
                  key_event->modifiers, key_event->keycode, nullptr, false);
            }
            return ECORE_CALLBACK_DONE;
          }
        }
        return ECORE_CALLBACK_PASS_ON;
      },
      this));
}

void TizenWindowEcoreWl2::UnregisterEventHandlers() {
  for (Ecore_Event_Handler* handler : ecore_event_handlers_) {
    ecore_event_handler_del(handler);
  }
  ecore_event_handlers_.clear();
}

void TizenWindowEcoreWl2::DestroyWindow() {
  if (ecore_wl2_egl_window_) {
    ecore_wl2_egl_window_destroy(ecore_wl2_egl_window_);
    ecore_wl2_egl_window_ = nullptr;
  }

  if (ecore_wl2_window_) {
    ecore_wl2_window_free(ecore_wl2_window_);
    ecore_wl2_window_ = nullptr;
  }

  if (ecore_wl2_display_) {
    ecore_wl2_display_disconnect(ecore_wl2_display_);
    ecore_wl2_display_ = nullptr;
  }
  ecore_wl2_shutdown();
}

TizenGeometry TizenWindowEcoreWl2::GetGeometry() {
  TizenGeometry result;
  ecore_wl2_window_geometry_get(ecore_wl2_window_, &result.left, &result.top,
                                &result.width, &result.height);
  return result;
}

bool TizenWindowEcoreWl2::SetGeometry(TizenGeometry geometry) {
  ecore_wl2_window_rotation_geometry_set(ecore_wl2_window_, GetRotation(),
                                         geometry.left, geometry.top,
                                         geometry.width, geometry.height);
  // FIXME: The changes set in `ecore_wl2_window_geometry_set` seems to apply
  // only after calling `ecore_wl2_window_position_set`. Call a more appropriate
  // API that flushes geometry settings to the compositor.
  ecore_wl2_window_position_set(ecore_wl2_window_, geometry.left, geometry.top);
  return true;
}

TizenGeometry TizenWindowEcoreWl2::GetScreenGeometry() {
  TizenGeometry result = {};
  ecore_wl2_display_screen_size_get(ecore_wl2_display_, &result.width,
                                    &result.height);
  return result;
}

int32_t TizenWindowEcoreWl2::GetRotation() {
  return ecore_wl2_window_rotation_get(ecore_wl2_window_);
}

int32_t TizenWindowEcoreWl2::GetDpi() {
  Ecore_Wl2_Output* output = ecore_wl2_window_output_find(ecore_wl2_window_);
  if (!output) {
    FT_LOG(Error) << "Could not find an output associated with the window.";
    return 0;
  }
  return ecore_wl2_output_dpi_get(output);
}

uintptr_t TizenWindowEcoreWl2::GetWindowId() {
  return ecore_wl2_window_id_get(ecore_wl2_window_);
}

void HandleResourceId(void* data, tizen_resource* tizen_resource, uint32_t id) {
  if (data) {
    *reinterpret_cast<uint32_t*>(data) = id;
  }
}

uint32_t TizenWindowEcoreWl2::GetResourceId() {
  if (resource_id_ > 0) {
    return resource_id_;
  }
  struct wl_registry* registry =
      ecore_wl2_display_registry_get(ecore_wl2_display_);
  if (!registry) {
    FT_LOG(Error) << "Could not retreive wl_registry from the display.";
    return 0;
  }

  static const struct tizen_resource_listener tz_resource_listener = {
      HandleResourceId};
  Eina_Iterator* iter = ecore_wl2_display_globals_get(ecore_wl2_display_);
  Ecore_Wl2_Global* global = nullptr;
  struct tizen_surface* surface = nullptr;
  EINA_ITERATOR_FOREACH(iter, global) {
    if (strcmp(global->interface, "tizen_surface") == 0) {
      surface = static_cast<tizen_surface*>(wl_registry_bind(
          registry, global->id, &tizen_surface_interface, global->version));
      break;
    }
  }
  eina_iterator_free(iter);
  if (!surface) {
    FT_LOG(Error) << "Failed to initialize the tizen surface.";
    return 0;
  }

  struct tizen_resource* resource = tizen_surface_get_tizen_resource(
      surface, ecore_wl2_window_surface_get(ecore_wl2_window_));

  if (!resource) {
    FT_LOG(Error) << "Failed to get tizen resource.";
    tizen_surface_destroy(surface);
    return 0;
  }

  struct wl_event_queue* event_queue = wl_display_create_queue(wl2_display_);
  if (!event_queue) {
    FT_LOG(Error) << "Failed to create wl_event_queue.";
    tizen_resource_destroy(resource);
    tizen_surface_destroy(surface);
    return 0;
  }
  wl_proxy_set_queue(reinterpret_cast<struct wl_proxy*>(resource), event_queue);
  tizen_resource_add_listener(resource, &tz_resource_listener, &resource_id_);
  wl_display_roundtrip_queue(wl2_display_, event_queue);
  tizen_resource_destroy(resource);
  tizen_surface_destroy(surface);
  wl_event_queue_destroy(event_queue);
  return resource_id_;
}

void TizenWindowEcoreWl2::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  ecore_wl2_window_available_rotations_set(ecore_wl2_window_, rotations.data(),
                                           rotations.size());
}

void TizenWindowEcoreWl2::BindKeys(const std::vector<std::string>& keys) {
  for (const std::string& key : keys) {
    ecore_wl2_window_keygrab_set(ecore_wl2_window_, key.c_str(), 0, 0, 0,
                                 ECORE_WL2_WINDOW_KEYGRAB_TOPMOST);
  }
}

void TizenWindowEcoreWl2::Show() {
  ecore_wl2_window_show(ecore_wl2_window_);
}

void TizenWindowEcoreWl2::UpdateFlutterCursor(const std::string& kind) {
#ifdef TV_PROFILE
  int pointer_size = -1;
  if (vconf_get_int(kSysMouseCursorPointerSizeVConfKey, &pointer_size) < 0) {
    FT_LOG(Info) << "Failed to load cursor size.";
  }

  std::string cursor_name = "normal_default";
  if (kind == "basic") {
    if (pointer_size == 0) {  // Large.
      cursor_name = "large_normal";
    } else if (pointer_size == 1) {  // Medium.
      cursor_name = "medium_normal";
    } else if (pointer_size == 2) {  // Small.
      cursor_name = "small_normal";
    } else {
      cursor_name = "normal_default";
    }
  } else if (kind == "click") {
    if (pointer_size == 0) {  // Large.
      cursor_name = "large_normal_pnh";
    } else if (pointer_size == 1) {  // Medium.
      cursor_name = "medium_normal_pnh";
    } else if (pointer_size == 2) {  // Small.
      cursor_name = "small_normal_pnh";
    } else {
      cursor_name = "normal_pnh";
    }
  } else if (kind == "text") {
    if (pointer_size == 0) {  // Large.
      cursor_name = "large_normal_input_field";
    } else if (pointer_size == 1) {  // Medium.
      cursor_name = "medium_normal_input_field";
    } else if (pointer_size == 2) {  // Small.
      cursor_name = "small_normal_input_field";
    } else {
      cursor_name = "normal_input_field";
    }
  } else if (kind == "none") {
    cursor_name = "normal_transparent";
  } else {
    FT_LOG(Info) << kind << " cursor is not supported.";
  }
  ecore_wl2_input_cursor_theme_name_set(
      ecore_wl2_input_default_input_get(ecore_wl2_display_),
      kEcoreWL2InputCursorThemeName);
  ecore_wl2_input_cursor_from_name_set(
      ecore_wl2_input_default_input_get(ecore_wl2_display_),
      cursor_name.c_str());
#else
  FT_LOG(Info) << "UpdateFlutterCursor is not supported.";
#endif
}

void TizenWindowEcoreWl2::SetTizenPolicyNotificationLevel(int level) {
  wl_registry* registry = ecore_wl2_display_registry_get(ecore_wl2_display_);
  if (!registry) {
    FT_LOG(Error) << "Could not retreive wl_registry from the display.";
    return;
  }

  Eina_Iterator* iter = ecore_wl2_display_globals_get(ecore_wl2_display_);
  Ecore_Wl2_Global* global = nullptr;

  // Retrieve global objects to bind a tizen policy.
  EINA_ITERATOR_FOREACH(iter, global) {
    if (strcmp(global->interface, tizen_policy_interface.name) == 0) {
      tizen_policy_ = static_cast<tizen_policy*>(
          wl_registry_bind(registry, global->id, &tizen_policy_interface, 1));
      break;
    }
  }
  eina_iterator_free(iter);

  if (!tizen_policy_) {
    FT_LOG(Error)
        << "Failed to initialize the tizen policy handle, the top_level "
           "attribute is ignored.";
    return;
  }

  tizen_policy_set_notification_level(
      tizen_policy_, ecore_wl2_window_surface_get(ecore_wl2_window_), level);
}

void TizenWindowEcoreWl2::PrepareInputMethod() {
  input_method_context_ =
      std::make_unique<TizenInputMethodContext>(GetWindowId());

  // Set input method callbacks.
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

}  // namespace flutter
