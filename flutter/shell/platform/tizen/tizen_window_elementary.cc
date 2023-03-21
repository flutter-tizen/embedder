// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_window_elementary.h"

#include <efl_extension.h>
#include <eom.h>
#include <ui/efl_util.h>

#include <memory>

#include "flutter/shell/platform/tizen/logger.h"
#ifndef WEARABLE_PROFILE
#include "flutter/shell/platform/tizen/tizen_autofill.h"
#endif
#include "flutter/shell/platform/tizen/tizen_view_event_handler_delegate.h"

#if defined(MOBILE_PROFILE)
constexpr double kProfileFactor = 0.7;
#elif defined(TV_PROFILE)
constexpr double kProfileFactor = 2.0;
#elif not defined(WEARABLE_PROFILE)
constexpr double kProfileFactor = 1.0;
#endif

namespace flutter {

namespace {

constexpr int kScrollDirectionVertical = 0;
constexpr int kScrollDirectionHorizontal = 1;
constexpr int kScrollOffsetMultiplier = 20;

uint32_t EvasModifierToEcoreEventModifiers(const Evas_Modifier* evas_modifier) {
  uint32_t modifiers = 0;
  if (evas_key_modifier_is_set(evas_modifier, "Control")) {
    modifiers |= ECORE_EVENT_MODIFIER_CTRL;
  }
  if (evas_key_modifier_is_set(evas_modifier, "Alt")) {
    modifiers |= ECORE_EVENT_MODIFIER_ALT;
  }
  if (evas_key_modifier_is_set(evas_modifier, "Shift")) {
    modifiers |= ECORE_EVENT_MODIFIER_SHIFT;
  }
  return modifiers;
}

}  // namespace

TizenWindowElementary::TizenWindowElementary(
    TizenGeometry geometry,
    bool transparent,
    bool focusable,
    bool top_level,
    FlutterDesktopExternalOutputType external_output_type)
    : TizenWindow(geometry, transparent, focusable, top_level),
      external_output_type_(external_output_type) {
  if (external_output_type_ != FlutterDesktopExternalOutputType::kNone &&
      !InitializeExternalOutputManager()) {
    FT_LOG(Error) << "Failed to initialize the External Output Manager.";
    return;
  }

  if (!CreateWindow()) {
    FT_LOG(Error) << "Failed to create a platform window.";
    return;
  }

  SetWindowOptions();
  RegisterEventHandlers();
#ifndef WEARABLE_PROFILE
  PrepareAutofill();
#endif
  PrepareInputMethod();
  Show();
}

TizenWindowElementary::~TizenWindowElementary() {
  if (external_output_type_ != FlutterDesktopExternalOutputType::kNone) {
    DestroyExternalOutputManager();
  }
  UnregisterEventHandlers();
  DestroyWindow();
}

bool TizenWindowElementary::CreateWindow() {
  elm_config_accel_preference_set("hw:opengl");

  elm_win_ = elm_win_add(nullptr, nullptr,
                         top_level_ ? ELM_WIN_NOTIFICATION : ELM_WIN_BASIC);
  if (!elm_win_) {
    FT_LOG(Error) << "Could not create an Evas window.";
    return false;
  }

#ifndef WEARABLE_PROFILE
  elm_win_aux_hint_add(elm_win_, "wm.policy.win.user.geometry", "1");
#endif

  int32_t width = 0, height = 0;
  if (external_output_type_ != FlutterDesktopExternalOutputType::kNone) {
    eom_get_output_resolution(external_output_id_, &width, &height);
  } else {
    Ecore_Evas* ecore_evas =
        ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_));
    ecore_evas_screen_geometry_get(ecore_evas, nullptr, nullptr, &width,
                                   &height);
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

  evas_object_move(elm_win_, initial_geometry_.left, initial_geometry_.top);
  evas_object_resize(elm_win_, initial_geometry_.width,
                     initial_geometry_.height);
  evas_object_raise(elm_win_);

#ifndef WEARABLE_PROFILE
  ctxpopup_ = elm_ctxpopup_add(elm_win_);
  elm_ctxpopup_direction_priority_set(
      ctxpopup_, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_RIGHT,
      ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_UP);
#endif

  image_ = evas_object_image_filled_add(evas_object_evas_get(elm_win_));
  evas_object_resize(image_, initial_geometry_.width, initial_geometry_.height);
  evas_object_move(image_, initial_geometry_.left, initial_geometry_.top);
  evas_object_image_size_set(image_, initial_geometry_.width,
                             initial_geometry_.height);
  evas_object_image_alpha_set(image_, EINA_TRUE);
  elm_win_resize_object_add(elm_win_, image_);

  if (external_output_type_ != FlutterDesktopExternalOutputType::kNone) {
    if (eom_set_output_window(external_output_id_, elm_win_) !=
        EOM_ERROR_NONE) {
      FT_LOG(Error) << "eom_set_output_window() failed.";
      return false;
    }
  }

  return elm_win_ && image_;
}

void TizenWindowElementary::DestroyWindow() {
  evas_object_del(elm_win_);
  evas_object_del(image_);
#ifndef WEARABLE_PROFILE
  evas_object_del(ctxpopup_);
#endif
}

void TizenWindowElementary::SetWindowOptions() {
  if (top_level_) {
    efl_util_set_notification_window_level(elm_win_,
                                           EFL_UTIL_NOTIFICATION_LEVEL_TOP);
  }

  if (transparent_) {
    elm_win_alpha_set(elm_win_, EINA_TRUE);
  } else {
    elm_win_alpha_set(elm_win_, EINA_FALSE);

    Evas_Object* bg = elm_bg_add(elm_win_);
    evas_object_color_set(bg, 0, 0, 0, 0);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(elm_win_, bg);
  }

  elm_win_indicator_mode_set(elm_win_, ELM_WIN_INDICATOR_SHOW);
  elm_win_indicator_opacity_set(elm_win_, ELM_WIN_INDICATOR_OPAQUE);

  // TODO: focusable_

  const int rotations[4] = {0, 90, 180, 270};
  elm_win_wm_rotation_available_rotations_set(elm_win_, &rotations[0], 4);
}

void TizenWindowElementary::RegisterEventHandlers() {
  rotation_changed_callback_ = [](void* data, Evas_Object* object,
                                  void* event_info) {
    auto* self = static_cast<TizenWindowElementary*>(data);
    if (self->view_delegate_) {
      if (self->elm_win_ == object) {
        self->view_delegate_->OnRotate(self->GetRotation());
        elm_win_wm_rotation_manual_rotation_done(self->elm_win_);
      }
    }
  };
  evas_object_smart_callback_add(elm_win_, "rotation,changed",
                                 rotation_changed_callback_, this);

  evas_object_callbacks_[EVAS_CALLBACK_RESIZE] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenWindowElementary*>(data);
        if (self->view_delegate_) {
          if (self->elm_win_ == object) {
            int32_t x = 0, y = 0, width = 0, height = 0;
            evas_object_geometry_get(object, &x, &y, &width, &height);

            evas_object_resize(self->image_, width, height);
            evas_object_move(self->image_, x, y);

            self->view_delegate_->OnResize(0, 0, width, height);
          }
        }
      };
  evas_object_event_callback_add(elm_win_, EVAS_CALLBACK_RESIZE,
                                 evas_object_callbacks_[EVAS_CALLBACK_RESIZE],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenWindowElementary*>(data);
        if (self->view_delegate_) {
          if (self->image_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Down*>(event_info);
            self->view_delegate_->OnPointerDown(
                mouse_event->canvas.x, mouse_event->canvas.y,
                mouse_event->timestamp, kFlutterPointerDeviceKindTouch,
                mouse_event->button);
          }
        }
      };
  evas_object_event_callback_add(
      image_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = static_cast<TizenWindowElementary*>(data);
    if (self->view_delegate_) {
      if (self->image_ == object) {
        auto* mouse_event = reinterpret_cast<Evas_Event_Mouse_Up*>(event_info);
        self->view_delegate_->OnPointerUp(
            mouse_event->canvas.x, mouse_event->canvas.y,
            mouse_event->timestamp, kFlutterPointerDeviceKindTouch,
            mouse_event->button);
      }
    }
  };
  evas_object_event_callback_add(image_, EVAS_CALLBACK_MOUSE_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenWindowElementary*>(data);
        if (self->view_delegate_) {
          if (self->image_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Move*>(event_info);
            self->view_delegate_->OnPointerMove(
                mouse_event->cur.canvas.x, mouse_event->cur.canvas.y,
                mouse_event->timestamp, kFlutterPointerDeviceKindTouch,
                mouse_event->buttons);
          }
        }
      };
  evas_object_event_callback_add(
      image_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenWindowElementary*>(data);
        if (self->view_delegate_) {
          if (self->image_ == object) {
            auto* wheel_event =
                reinterpret_cast<Ecore_Event_Mouse_Wheel*>(event_info);
            double delta_x = 0.0;
            double delta_y = 0.0;

            if (wheel_event->direction == kScrollDirectionVertical) {
              delta_y += wheel_event->z;
            } else if (wheel_event->direction == kScrollDirectionHorizontal) {
              delta_x += wheel_event->z;
            }

            self->view_delegate_->OnScroll(
                wheel_event->x, wheel_event->y, delta_x, delta_y,
                kScrollOffsetMultiplier, wheel_event->timestamp,
                kFlutterPointerDeviceKindTouch, 0);
          }
        }
      };
  evas_object_event_callback_add(
      image_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL], this);

  evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = static_cast<TizenWindowElementary*>(data);
    if (self->view_delegate_) {
      if (self->elm_win_ == object) {
        auto* key_event = reinterpret_cast<Evas_Event_Key_Down*>(event_info);
        bool handled = false;
        if (self->input_method_context_->IsInputPanelShown()) {
          handled =
              self->input_method_context_->HandleEvasEventKeyDown(key_event);
        }
        if (!handled) {
          self->view_delegate_->OnKey(
              key_event->key, key_event->string, key_event->compose,
              EvasModifierToEcoreEventModifiers(key_event->modifiers),
              key_event->keycode, true);
        }
      }
    }
  };
  evas_object_event_callback_add(elm_win_, EVAS_CALLBACK_KEY_DOWN,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_KEY_UP] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenWindowElementary*>(data);
        if (self->view_delegate_) {
          if (self->elm_win_ == object) {
            auto* key_event = reinterpret_cast<Evas_Event_Key_Up*>(event_info);
            bool handled = false;
            if (self->input_method_context_->IsInputPanelShown()) {
              handled =
                  self->input_method_context_->HandleEvasEventKeyUp(key_event);
            }
            if (!handled) {
              self->view_delegate_->OnKey(
                  key_event->key, key_event->string, key_event->compose,
                  EvasModifierToEcoreEventModifiers(key_event->modifiers),
                  key_event->keycode, false);
            }
          }
        }
      };
  evas_object_event_callback_add(elm_win_, EVAS_CALLBACK_KEY_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_UP],
                                 this);

#ifndef WEARABLE_PROFILE
  evas_object_callbacks_[EVAS_CALLBACK_HIDE] =
      [](void* data, Evas* e, Evas_Object* obj, void* event_info) {
        elm_ctxpopup_clear(obj);
      };
  evas_object_event_callback_add(ctxpopup_, EVAS_CALLBACK_HIDE,
                                 evas_object_callbacks_[EVAS_CALLBACK_HIDE],
                                 nullptr);
#endif
}

void TizenWindowElementary::UnregisterEventHandlers() {
  evas_object_smart_callback_del(elm_win_, "rotation,changed",
                                 rotation_changed_callback_);

  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN]);
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_UP,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP]);
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE]);
  evas_object_event_callback_del(
      image_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL]);

  evas_object_event_callback_del(
      elm_win_, EVAS_CALLBACK_KEY_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN]);
  evas_object_event_callback_del(elm_win_, EVAS_CALLBACK_KEY_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_UP]);

#ifndef WEARABLE_PROFILE
  evas_object_event_callback_del(ctxpopup_, EVAS_CALLBACK_HIDE,
                                 evas_object_callbacks_[EVAS_CALLBACK_HIDE]);
#endif
}

TizenGeometry TizenWindowElementary::GetGeometry() {
  // FIXME : evas_object_geometry_get() and ecore_wl2_window_geometry_get() are
  // not equivalent.
  TizenGeometry result;
  evas_object_geometry_get(elm_win_, &result.left, &result.top, &result.width,
                           &result.height);
  return result;
}

bool TizenWindowElementary::SetGeometry(TizenGeometry geometry) {
#ifndef WEARABLE_PROFILE
  evas_object_resize(elm_win_, geometry.width, geometry.height);
  evas_object_move(elm_win_, geometry.left, geometry.top);
  return true;
#else
  FT_LOG(Error) << "SetGeometry is not supported.";
  return false;
#endif
}

TizenGeometry TizenWindowElementary::GetScreenGeometry() {
  TizenGeometry result;
  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_));
  ecore_evas_screen_geometry_get(ecore_evas, nullptr, nullptr, &result.width,
                                 &result.height);
  return result;
}

int32_t TizenWindowElementary::GetRotation() {
  return elm_win_rotation_get(elm_win_);
}

int32_t TizenWindowElementary::GetDpi() {
  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_));
  int32_t xdpi, ydpi;
  ecore_evas_screen_dpi_get(ecore_evas, &xdpi, &ydpi);
  return xdpi;
}

uintptr_t TizenWindowElementary::GetWindowId() {
  return ecore_evas_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(elm_win_)));
}

void TizenWindowElementary::SetPreferredOrientations(
    const std::vector<int>& rotations) {
  elm_win_wm_rotation_available_rotations_set(elm_win_, rotations.data(),
                                              rotations.size());
}

void TizenWindowElementary::BindKeys(const std::vector<std::string>& keys) {
  for (const std::string& key : keys) {
    eext_win_keygrab_set(elm_win_, key.c_str());
  }
}

void TizenWindowElementary::Show() {
  evas_object_show(image_);
  evas_object_show(elm_win_);
}

#ifndef WEARABLE_PROFILE
void TizenWindowElementary::PrepareAutofill() {
  TizenAutofill& autofill = TizenAutofill::GetInstance();
  autofill.SetOnPopup([this]() {
    const std::vector<std::unique_ptr<AutofillItem>>& items =
        TizenAutofill::GetInstance().GetResponseItems();
    if (items.empty()) {
      return;
    }
    for (const auto& item : items) {
      elm_ctxpopup_item_append(
          ctxpopup_, item->label.c_str(), nullptr,
          [](void* data, Evas_Object* obj, void* event_info) {
            AutofillItem* item = static_cast<AutofillItem*>(data);
            TizenAutofill::GetInstance().OnCommit(item->value);
            evas_object_hide(obj);
          },
          item.get());
    }
// TODO(Swanseo0): Change ctxpopup's position to focused input field.
#ifdef TV_PROFILE
    double_t dpi = 72.0;
#else
    double_t dpi = static_cast<double>(GetDpi());
#endif
    double_t scale_factor = dpi / 90.0 * kProfileFactor;
    InputFieldGeometry geometry =
        input_method_context_->GetInputFieldGeometry();
    evas_object_move(ctxpopup_, geometry.x * scale_factor,
                     geometry.y * scale_factor);
    evas_object_show(ctxpopup_);
  });
}
#endif

void TizenWindowElementary::PrepareInputMethod() {
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

int32_t TizenWindowElementary::GetExternalOutputId() {
  int32_t num_ids = 0;
  eom_output_id* output_ids = eom_get_eom_output_ids(&num_ids);
  if (!output_ids || num_ids == 0) {
    FT_LOG(Error) << "No external output found.";
    return 0;
  }

  eom_output_id output_id = 0;
  for (int32_t i = 0; i < num_ids; i++) {
    eom_output_type_e output_type = EOM_OUTPUT_TYPE_UNKNOWN;
    eom_get_output_type(output_ids[i], &output_type);

    if (external_output_type_ == FlutterDesktopExternalOutputType::kHDMI &&
        (output_type == EOM_OUTPUT_TYPE_HDMIA ||
         output_type == EOM_OUTPUT_TYPE_HDMIB)) {
      output_id = output_ids[i];
      break;
    }
  }
  free(output_ids);
  return output_id;
}

bool TizenWindowElementary::InitializeExternalOutputManager() {
  if (eom_init()) {
    FT_LOG(Error) << "eom_init() failed.";
    return false;
  }

  external_output_id_ = GetExternalOutputId();
  if (external_output_id_ == 0) {
    FT_LOG(Error) << "Invalid external output ID.";
    return false;
  }

  int ret = eom_set_output_attribute(external_output_id_,
                                     EOM_OUTPUT_ATTRIBUTE_NORMAL);
  if (ret != EOM_ERROR_NONE) {
    FT_LOG(Error)
        << "eom_set_output_attribute() failed. Cannot use external output.";
    return false;
  }
  return true;
}

void TizenWindowElementary::DestroyExternalOutputManager() {
  eom_deinit();
}

}  // namespace flutter
