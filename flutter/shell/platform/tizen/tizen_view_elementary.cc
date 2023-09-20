// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_view_elementary.h"

#include <memory>
#include <string>

#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_view_event_handler_delegate.h"

namespace flutter {

namespace {

constexpr int kScrollDirectionVertical = 0;
constexpr int kScrollDirectionHorizontal = 1;

FlutterPointerMouseButtons ToFlutterPointerButton(int32_t button) {
  if (button == 2) {
    return kFlutterPointerButtonMouseMiddle;
  } else if (button == 3) {
    return kFlutterPointerButtonMouseSecondary;
  } else {
    return kFlutterPointerButtonMousePrimary;
  }
}

FlutterPointerDeviceKind ToFlutterDeviceKind(const Evas_Device* dev) {
  Evas_Device_Class device_class = evas_device_class_get(dev);
  if (device_class == EVAS_DEVICE_CLASS_MOUSE) {
    return kFlutterPointerDeviceKindMouse;
  } else if (device_class == EVAS_DEVICE_CLASS_PEN) {
    return kFlutterPointerDeviceKindStylus;
  } else {
    return kFlutterPointerDeviceKindTouch;
  }
}

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

void EvasObjectResizeWithMinMaxHint(Evas_Object* object,
                                    int32_t width,
                                    int32_t height) {
  evas_object_resize(object, width, height);
  evas_object_size_hint_min_set(object, width, height);
  evas_object_size_hint_max_set(object, width, height);
}

}  // namespace

TizenViewElementary::TizenViewElementary(int32_t width,
                                         int32_t height,
                                         Evas_Object* parent)
    : TizenView(width, height), parent_(parent) {
  if (!CreateView()) {
    FT_LOG(Error) << "Failed to create a platform view.";
    return;
  }

  RegisterEventHandlers();
  PrepareInputMethod();
  Show();
}

TizenViewElementary::~TizenViewElementary() {
  UnregisterEventHandlers();
  DestroyView();
}

bool TizenViewElementary::CreateView() {
  elm_config_accel_preference_set("hw:opengl");

  int32_t parent_width, parent_height;
  evas_object_geometry_get(parent_, nullptr, nullptr, &parent_width,
                           &parent_height);

  if (initial_width_ == 0) {
    initial_width_ = parent_width;
  }
  if (initial_height_ == 0) {
    initial_height_ = parent_height;
  }

  container_ = elm_bg_add(parent_);
  if (!container_) {
    FT_LOG(Error) << "Failed to create an Evas object container.";
    return false;
  }
  evas_object_size_hint_align_set(container_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  EvasObjectResizeWithMinMaxHint(container_, initial_width_, initial_height_);
  elm_object_focus_allow_set(container_, EINA_TRUE);

  image_ = evas_object_image_filled_add(evas_object_evas_get(container_));
  if (!image_) {
    FT_LOG(Error) << "Failed to create an Evas object image.";
    return false;
  }
  evas_object_size_hint_align_set(image_, EVAS_HINT_FILL, EVAS_HINT_FILL);
  EvasObjectResizeWithMinMaxHint(image_, initial_width_, initial_height_);
  evas_object_image_size_set(image_, initial_width_, initial_height_);
  evas_object_image_alpha_set(image_, EINA_TRUE);
  elm_object_part_content_set(container_, "overlay", image_);
  return true;
}

void TizenViewElementary::DestroyView() {
  evas_object_del(image_);
  evas_object_del(container_);
}

void TizenViewElementary::RegisterEventHandlers() {
  evas_object_callbacks_[EVAS_CALLBACK_RESIZE] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenViewElementary*>(data);
        if (self->view_delegate_) {
          if (self->container_ == object) {
            int32_t width = 0, height = 0;
            evas_object_geometry_get(self->container_, nullptr, nullptr, &width,
                                     &height);

            evas_object_size_hint_min_set(self->container_, width, height);
            evas_object_size_hint_max_set(self->container_, width, height);

            EvasObjectResizeWithMinMaxHint(self->image_, width, height);

            self->view_delegate_->OnResize(0, 0, width, height);
          }
        }
      };
  evas_object_event_callback_add(container_, EVAS_CALLBACK_RESIZE,
                                 evas_object_callbacks_[EVAS_CALLBACK_RESIZE],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenViewElementary*>(data);
        if (self->view_delegate_) {
          if (self->container_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Down*>(event_info);
            TizenGeometry geometry = self->GetGeometry();
            self->view_delegate_->OnPointerDown(
                mouse_event->canvas.x - geometry.left,
                mouse_event->canvas.y - geometry.top,
                ToFlutterPointerButton(mouse_event->button),
                mouse_event->timestamp, ToFlutterDeviceKind(mouse_event->dev),
                reinterpret_cast<intptr_t>(mouse_event->dev));
          }
        }
      };
  evas_object_event_callback_add(
      container_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = static_cast<TizenViewElementary*>(data);
    if (self->view_delegate_) {
      if (self->container_ == object) {
        auto* mouse_event = reinterpret_cast<Evas_Event_Mouse_Up*>(event_info);
        if (self->scroll_hold_) {
          elm_object_scroll_hold_pop(self->container_);
          self->scroll_hold_ = false;
        }
        TizenGeometry geometry = self->GetGeometry();
        self->view_delegate_->OnPointerUp(
            mouse_event->canvas.x - geometry.left,
            mouse_event->canvas.y - geometry.top,
            ToFlutterPointerButton(mouse_event->button), mouse_event->timestamp,
            ToFlutterDeviceKind(mouse_event->dev),
            reinterpret_cast<intptr_t>(mouse_event->dev));
      }
    }
  };
  evas_object_event_callback_add(container_, EVAS_CALLBACK_MOUSE_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenViewElementary*>(data);
        if (self->view_delegate_) {
          if (self->container_ == object) {
            auto* mouse_event =
                reinterpret_cast<Evas_Event_Mouse_Move*>(event_info);
            mouse_event->event_flags = Evas_Event_Flags(
                mouse_event->event_flags | EVAS_EVENT_FLAG_ON_HOLD);
            if (!self->scroll_hold_) {
              elm_object_scroll_hold_push(self->container_);
              self->scroll_hold_ = true;
            }

            TizenGeometry geometry = self->GetGeometry();
            self->view_delegate_->OnPointerMove(
                mouse_event->cur.canvas.x - geometry.left,
                mouse_event->cur.canvas.y - geometry.top,
                mouse_event->timestamp, ToFlutterDeviceKind(mouse_event->dev),
                reinterpret_cast<intptr_t>(mouse_event->dev));
          }
        }
      };
  evas_object_event_callback_add(
      container_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE], this);

  evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenViewElementary*>(data);
        if (self->view_delegate_) {
          if (self->container_ == object) {
            auto* wheel_event =
                reinterpret_cast<Ecore_Event_Mouse_Wheel*>(event_info);
            double delta_x = 0.0;
            double delta_y = 0.0;

            if (wheel_event->direction == kScrollDirectionVertical) {
              delta_y += wheel_event->z;
            } else if (wheel_event->direction == kScrollDirectionHorizontal) {
              delta_x += wheel_event->z;
            }

            TizenGeometry geometry = self->GetGeometry();
            self->view_delegate_->OnScroll(
                wheel_event->x - geometry.left, wheel_event->y - geometry.top,
                delta_x, delta_y, wheel_event->timestamp,
                ToFlutterDeviceKind(wheel_event->dev),
                reinterpret_cast<intptr_t>(wheel_event->dev));
          }
        }
      };
  evas_object_event_callback_add(
      container_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL], this);

  evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN] = [](void* data, Evas* evas,
                                                      Evas_Object* object,
                                                      void* event_info) {
    auto* self = static_cast<TizenViewElementary*>(data);
    if (self->view_delegate_) {
      if (self->container_ == object && self->focused_) {
        auto* key_event = reinterpret_cast<Evas_Event_Key_Down*>(event_info);
        bool handled = false;
        key_event->event_flags =
            Evas_Event_Flags(key_event->event_flags | EVAS_EVENT_FLAG_ON_HOLD);
        if (self->input_method_context_->IsInputPanelShown()) {
          handled =
              self->input_method_context_->HandleEvasEventKeyDown(key_event);
        }
        if (!handled) {
          self->view_delegate_->OnKey(
              key_event->key, key_event->string, key_event->compose,
              EvasModifierToEcoreEventModifiers(key_event->modifiers),
              key_event->keycode, evas_device_name_get(key_event->dev), true);
        }
      }
    }
  };
  evas_object_event_callback_add(container_, EVAS_CALLBACK_KEY_DOWN,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN],
                                 this);

  evas_object_callbacks_[EVAS_CALLBACK_KEY_UP] =
      [](void* data, Evas* evas, Evas_Object* object, void* event_info) {
        auto* self = static_cast<TizenViewElementary*>(data);
        if (self->view_delegate_) {
          if (self->container_ == object && self->focused_) {
            auto* key_event = reinterpret_cast<Evas_Event_Key_Up*>(event_info);
            bool handled = false;
            key_event->event_flags = Evas_Event_Flags(key_event->event_flags |
                                                      EVAS_EVENT_FLAG_ON_HOLD);
            if (self->input_method_context_->IsInputPanelShown()) {
              handled =
                  self->input_method_context_->HandleEvasEventKeyUp(key_event);
            }
            if (!handled) {
              self->view_delegate_->OnKey(
                  key_event->key, key_event->string, key_event->compose,
                  EvasModifierToEcoreEventModifiers(key_event->modifiers),
                  key_event->keycode, nullptr, false);
            }
          }
        }
      };
  evas_object_event_callback_add(container_, EVAS_CALLBACK_KEY_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_UP],
                                 this);

  focused_callback_ = [](void* data, Evas_Object* object, void* event_info) {
    auto* self = static_cast<TizenViewElementary*>(data);
    if (self->view_delegate_) {
      if (self->container_ == object) {
        self->focused_ = true;
      }
    }
  };
  evas_object_smart_callback_add(container_, "focused", focused_callback_,
                                 this);
}

void TizenViewElementary::UnregisterEventHandlers() {
  evas_object_event_callback_del(container_, EVAS_CALLBACK_RESIZE,
                                 evas_object_callbacks_[EVAS_CALLBACK_RESIZE]);
  evas_object_event_callback_del(
      container_, EVAS_CALLBACK_MOUSE_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_DOWN]);
  evas_object_event_callback_del(
      container_, EVAS_CALLBACK_MOUSE_UP,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_UP]);
  evas_object_event_callback_del(
      container_, EVAS_CALLBACK_MOUSE_MOVE,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_MOVE]);
  evas_object_event_callback_del(
      container_, EVAS_CALLBACK_MOUSE_WHEEL,
      evas_object_callbacks_[EVAS_CALLBACK_MOUSE_WHEEL]);
  evas_object_event_callback_del(
      container_, EVAS_CALLBACK_KEY_DOWN,
      evas_object_callbacks_[EVAS_CALLBACK_KEY_DOWN]);
  evas_object_event_callback_del(container_, EVAS_CALLBACK_KEY_UP,
                                 evas_object_callbacks_[EVAS_CALLBACK_KEY_UP]);
  evas_object_smart_callback_del(container_, "focused", focused_callback_);
}

TizenGeometry TizenViewElementary::GetGeometry() {
  TizenGeometry result;
  evas_object_geometry_get(container_, &result.left, &result.top, &result.width,
                           &result.height);
  return result;
}

bool TizenViewElementary::SetGeometry(TizenGeometry geometry) {
  EvasObjectResizeWithMinMaxHint(container_, geometry.width, geometry.height);
  evas_object_move(container_, geometry.left, geometry.top);
  return true;
}

int32_t TizenViewElementary::GetDpi() {
  Ecore_Evas* ecore_evas =
      ecore_evas_ecore_evas_get(evas_object_evas_get(container_));
  int32_t xdpi, ydpi;
  ecore_evas_screen_dpi_get(ecore_evas, &xdpi, &ydpi);
  return xdpi;
}

uintptr_t TizenViewElementary::GetWindowId() {
  return ecore_evas_window_get(
      ecore_evas_ecore_evas_get(evas_object_evas_get(container_)));
}

void TizenViewElementary::Show() {
  evas_object_show(container_);
  evas_object_show(image_);
}

void TizenViewElementary::UpdateFlutterCursor(const std::string& kind) {
  FT_LOG(Info) << "UpdateFlutterCursor is not supported.";
}

void TizenViewElementary::PrepareInputMethod() {
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
