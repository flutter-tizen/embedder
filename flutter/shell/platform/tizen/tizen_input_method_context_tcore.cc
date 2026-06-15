// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_input_method_context_tcore.h"

#include <tizen_core_wl.h>

#include "flutter/shell/platform/tizen/logger.h"

namespace {

tizen_core_imf_input_panel_layout_e TextInputTypeToImfInputPanelLayout(
    const std::string& text_input_type) {
  if (text_input_type == "TextInputType.text" ||
      text_input_type == "TextInputType.multiline") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  } else if (text_input_type == "TextInputType.number") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
  } else if (text_input_type == "TextInputType.phone") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
  } else if (text_input_type == "TextInputType.datetime") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_DATETIME;
  } else if (text_input_type == "TextInputType.emailAddress" ||
             text_input_type == "TextInputType.twitter") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
  } else if (text_input_type == "TextInputType.url" ||
             text_input_type == "TextInputType.webSearch") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_URL;
  } else if (text_input_type == "TextInputType.visiblePassword") {
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
  } else {
    FT_LOG(Warn) << "The requested input type " << text_input_type
                 << " is not supported.";
    return TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
  }
}

tizen_core_imf_keyboard_modifiers_e ModifiersToImfModifiers(
    unsigned int modifiers) {
  unsigned int imf_modifiers = TIZEN_CORE_IMF_KEYBOARD_MODIFIERS_NONE;
  if (modifiers & TIZEN_CORE_WL_MODIFIER_SHIFT) {
    imf_modifiers |= TIZEN_CORE_IMF_KEYBOARD_MODIFIERS_SHIFT;
  }
  if (modifiers & TIZEN_CORE_WL_MODIFIER_ALT) {
    imf_modifiers |= TIZEN_CORE_IMF_KEYBOARD_MODIFIERS_ALT;
  }
  if (modifiers & TIZEN_CORE_WL_MODIFIER_CTRL) {
    imf_modifiers |= TIZEN_CORE_IMF_KEYBOARD_MODIFIERS_CTRL;
  }
  if (modifiers & TIZEN_CORE_WL_MODIFIER_WIN) {
    imf_modifiers |= TIZEN_CORE_IMF_KEYBOARD_MODIFIERS_WIN;
  }
  if (modifiers & TIZEN_CORE_WL_MODIFIER_ALTGR) {
    imf_modifiers |= TIZEN_CORE_IMF_KEYBOARD_MODIFIERS_ALTGR;
  }
  return static_cast<tizen_core_imf_keyboard_modifiers_e>(imf_modifiers);
}

tizen_core_imf_keyboard_locks_e ModifiersToImfLocks(unsigned int modifiers) {
  unsigned int locks = TIZEN_CORE_IMF_KEYBOARD_LOCKS_NONE;
  if (modifiers & TIZEN_CORE_WL_MODIFIER_NUM) {
    locks |= TIZEN_CORE_IMF_KEYBOARD_LOCKS_NUM;
  }
  if (modifiers & TIZEN_CORE_WL_MODIFIER_CAPS) {
    locks |= TIZEN_CORE_IMF_KEYBOARD_LOCKS_CAPS;
  }
  if (modifiers & TIZEN_CORE_WL_MODIFIER_SCROLL) {
    locks |= TIZEN_CORE_IMF_KEYBOARD_LOCKS_SCROLL;
  }
  return static_cast<tizen_core_imf_keyboard_locks_e>(locks);
}

tizen_core_imf_event_key_h CreateImfKeyEventFromTcoreWlEvent(void* event) {
  auto* ev = static_cast<tizen_core_wl_event_input_base_h>(event);

  tizen_core_imf_event_key_h imf_key = nullptr;
  tizen_core_imf_event_key_create(&imf_key);
  if (!imf_key) {
    return nullptr;
  }

  char* keyname = nullptr;
  tizen_core_wl_event_key_get_keyname(ev, &keyname);
  if (keyname) {
    tizen_core_imf_event_key_set_keyname(imf_key, keyname);
    tizen_core_imf_event_key_set_key(imf_key, keyname);
    free(keyname);
  }

  char* keysymbol = nullptr;
  tizen_core_wl_event_key_get_keysymbol(ev, &keysymbol);
  if (keysymbol) {
    tizen_core_imf_event_key_set_string(imf_key, keysymbol);
    free(keysymbol);
  }

  char* compose = nullptr;
  tizen_core_wl_event_key_get_compose(ev, &compose);
  if (compose) {
    tizen_core_imf_event_key_set_compose(imf_key, compose);
    free(compose);
  }

  unsigned int keycode = 0;
  tizen_core_wl_event_key_get_keycode(ev, &keycode);
  tizen_core_imf_event_key_set_keycode(imf_key, keycode);

  unsigned int modifiers = 0;
  tizen_core_wl_event_key_get_modifiers(ev, &modifiers);
  tizen_core_imf_event_key_set_modifiers(imf_key,
                                         ModifiersToImfModifiers(modifiers));
  tizen_core_imf_event_key_set_locks(imf_key, ModifiersToImfLocks(modifiers));

  uint32_t timestamp = 0;
  tizen_core_wl_event_input_base_get_timestamp(ev, &timestamp);
  tizen_core_imf_event_key_set_timestamp(imf_key, timestamp);

  char* dev_identifier = nullptr;
  tizen_core_wl_event_input_base_get_device_identifier(ev, &dev_identifier);
  if (dev_identifier) {
    tizen_core_imf_event_key_set_device_name(imf_key, dev_identifier);
    free(dev_identifier);
  }

  return imf_key;
}

}  // namespace

namespace flutter {

TizenInputMethodContext::TizenInputMethodContext(uintptr_t window_id) {
  tizen_core_imf_init();

  if (tizen_core_imf_context_create(&imf_context_) !=
      TIZEN_CORE_IMF_ERROR_NONE) {
    FT_LOG(Error) << "Failed to create tizen_core_imf_context.";
    return;
  }

  tizen_core_imf_context_set_client_window(imf_context_,
                                           reinterpret_cast<void*>(window_id));
  SetContextOptions();
  SetInputPanelOptions();
  RegisterEventCallbacks();
  RegisterInputPanelEventCallback();
}

TizenInputMethodContext::~TizenInputMethodContext() {
  UnregisterInputPanelEventCallback();
  UnregisterEventCallbacks();

  if (imf_context_) {
    tizen_core_imf_context_destroy(imf_context_);
  }

  tizen_core_imf_shutdown();
}

bool TizenInputMethodContext::HandleTcoreWlEventKey(void* event, bool is_down) {
  FT_ASSERT(imf_context_);
  FT_ASSERT(event);

  tizen_core_imf_event_key_h imf_key = CreateImfKeyEventFromTcoreWlEvent(event);
  if (!imf_key) {
    return false;
  }

  bool filter_result = false;
  tizen_core_imf_context_filter_event(imf_context_,
                                      is_down
                                          ? TIZEN_CORE_IMF_EVENT_TYPE_KEY_DOWN
                                          : TIZEN_CORE_IMF_EVENT_TYPE_KEY_UP,
                                      imf_key, &filter_result);
  tizen_core_imf_event_key_destroy(imf_key);
  return filter_result;
}

#ifdef NUI_SUPPORT
bool TizenInputMethodContext::HandleNuiKeyEvent(const char* device_name,
                                                uint32_t device_class,
                                                uint32_t device_subclass,
                                                const char* key,
                                                const char* string,
                                                uint32_t modifiers,
                                                uint32_t scan_code,
                                                size_t timestamp,
                                                bool is_down) {
  tizen_core_imf_event_key_h imf_key = nullptr;
  tizen_core_imf_event_key_create(&imf_key);
  if (!imf_key) {
    return false;
  }

  if (key) {
    tizen_core_imf_event_key_set_keyname(imf_key, key);
    tizen_core_imf_event_key_set_key(imf_key, key);
  }
  if (string) {
    tizen_core_imf_event_key_set_string(imf_key, string);
  }

  tizen_core_imf_event_key_set_modifiers(imf_key,
                                         ModifiersToImfModifiers(modifiers));
  tizen_core_imf_event_key_set_locks(imf_key, ModifiersToImfLocks(modifiers));
  tizen_core_imf_event_key_set_keycode(imf_key, scan_code);
  tizen_core_imf_event_key_set_timestamp(imf_key, timestamp);

  if (device_name) {
    tizen_core_imf_event_key_set_device_name(imf_key, device_name);
  }
  tizen_core_imf_event_key_set_device_class(
      imf_key, static_cast<tizen_core_imf_device_class_e>(device_class));
  tizen_core_imf_event_key_set_device_subclass(
      imf_key, static_cast<tizen_core_imf_device_subclass_e>(device_subclass));

  bool filter_result = false;
  tizen_core_imf_context_filter_event(imf_context_,
                                      is_down
                                          ? TIZEN_CORE_IMF_EVENT_TYPE_KEY_DOWN
                                          : TIZEN_CORE_IMF_EVENT_TYPE_KEY_UP,
                                      imf_key, &filter_result);
  tizen_core_imf_event_key_destroy(imf_key);
  return filter_result;
}
#endif

InputPanelGeometry TizenInputMethodContext::GetInputPanelGeometry() {
  FT_ASSERT(imf_context_);
  InputPanelGeometry geometry;
  tizen_core_imf_context_get_input_panel_geometry(
      imf_context_, &geometry.x, &geometry.y, &geometry.w, &geometry.h);
  return geometry;
}

void TizenInputMethodContext::ResetInputMethodContext() {
  FT_ASSERT(imf_context_);
  tizen_core_imf_context_reset(imf_context_);
}

void TizenInputMethodContext::ShowInputPanel() {
  FT_ASSERT(imf_context_);
  tizen_core_imf_context_input_panel_show(imf_context_);
  tizen_core_imf_context_focus_in(imf_context_);
}

void TizenInputMethodContext::HideInputPanel() {
  FT_ASSERT(imf_context_);
  tizen_core_imf_context_focus_out(imf_context_);
  tizen_core_imf_context_input_panel_hide(imf_context_);
}

bool TizenInputMethodContext::IsInputPanelShown() {
  tizen_core_imf_input_panel_state_e state;
  tizen_core_imf_context_get_input_panel_state(imf_context_, &state);
  return state == TIZEN_CORE_IMF_INPUT_PANEL_STATE_SHOW;
}

void TizenInputMethodContext::SetInputPanelLayout(
    const std::string& input_type) {
  FT_ASSERT(imf_context_);
  tizen_core_imf_input_panel_layout_e panel_layout =
      TextInputTypeToImfInputPanelLayout(input_type);
  tizen_core_imf_context_set_input_panel_layout(imf_context_, panel_layout);
}

void TizenInputMethodContext::SetInputPanelLayoutVariation(bool is_signed,
                                                           bool is_decimal) {
  tizen_core_imf_layout_numberonly_variation_e variation;
  if (is_signed && is_decimal) {
    variation = TIZEN_CORE_IMF_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL;
  } else if (is_signed) {
    variation = TIZEN_CORE_IMF_LAYOUT_NUMBERONLY_VARIATION_SIGNED;
  } else if (is_decimal) {
    variation = TIZEN_CORE_IMF_LAYOUT_NUMBERONLY_VARIATION_DECIMAL;
  } else {
    variation = TIZEN_CORE_IMF_LAYOUT_NUMBERONLY_VARIATION_NORMAL;
  }
  tizen_core_imf_context_set_input_panel_layout_variation(imf_context_,
                                                          variation);
}

void TizenInputMethodContext::SetAutocapitalType(const std::string& type) {
  tizen_core_imf_autocapital_type_e autocapital_type =
      TIZEN_CORE_IMF_AUTOCAPITAL_TYPE_NONE;

  if (type == "TextCapitalization.characters") {
    autocapital_type = TIZEN_CORE_IMF_AUTOCAPITAL_TYPE_ALLCHARACTER;
  } else if (type == "TextCapitalization.words") {
    autocapital_type = TIZEN_CORE_IMF_AUTOCAPITAL_TYPE_WORD;
  } else if (type == "TextCapitalization.sentences") {
    autocapital_type = TIZEN_CORE_IMF_AUTOCAPITAL_TYPE_SENTENCE;
  } else if (type == "TextCapitalization.none") {
    autocapital_type = TIZEN_CORE_IMF_AUTOCAPITAL_TYPE_NONE;
  }
  tizen_core_imf_context_set_autocapital_type(imf_context_, autocapital_type);
}

void TizenInputMethodContext::RegisterEventCallbacks() {
  FT_ASSERT(imf_context_);

  // commit callback
  event_callbacks_[TIZEN_CORE_IMF_CALLBACK_COMMIT] =
      [](tizen_core_imf_context_h ctx, void* event_info, void* data) {
        auto* self = static_cast<TizenInputMethodContext*>(data);
        char* str = static_cast<char*>(event_info);
        if (self->on_commit_) {
          self->on_commit_(str);
        }
      };
  tizen_core_imf_context_add_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_COMMIT,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_COMMIT], this);

  // pre-edit start callback
  event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_START] =
      [](tizen_core_imf_context_h ctx, void* event_info, void* data) {
        auto* self = static_cast<TizenInputMethodContext*>(data);
        if (self->on_preedit_start_) {
          self->on_preedit_start_();
        }
      };
  tizen_core_imf_context_add_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_PREEDIT_START,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_START], this);

  // pre-edit end callback
  event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_END] =
      [](tizen_core_imf_context_h ctx, void* event_info, void* data) {
        auto* self = static_cast<TizenInputMethodContext*>(data);
        if (self->on_preedit_end_) {
          self->on_preedit_end_();
        }
      };
  tizen_core_imf_context_add_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_PREEDIT_END,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_END], this);

  // pre-edit changed callback
  event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED] =
      [](tizen_core_imf_context_h ctx, void* event_info, void* data) {
        auto* self = static_cast<TizenInputMethodContext*>(data);
        if (self->on_preedit_changed_) {
          char* str = nullptr;
          int cursor_pos = 0;
          tizen_core_imf_context_get_preedit_string(ctx, &str, nullptr, nullptr,
                                                    &cursor_pos);
          if (str) {
            self->on_preedit_changed_(str, cursor_pos);
            free(str);
          }
        }
      };
  tizen_core_imf_context_add_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED], this);
}

void TizenInputMethodContext::UnregisterEventCallbacks() {
  FT_ASSERT(imf_context_);
  tizen_core_imf_context_del_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_COMMIT,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_COMMIT]);
  tizen_core_imf_context_del_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_CHANGED]);
  tizen_core_imf_context_del_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_PREEDIT_START,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_START]);
  tizen_core_imf_context_del_event_callback(
      imf_context_, TIZEN_CORE_IMF_CALLBACK_PREEDIT_END,
      event_callbacks_[TIZEN_CORE_IMF_CALLBACK_PREEDIT_END]);
}

void TizenInputMethodContext::SetContextOptions() {
  FT_ASSERT(imf_context_);
  tizen_core_imf_context_set_autocapital_type(
      imf_context_, TIZEN_CORE_IMF_AUTOCAPITAL_TYPE_NONE);
}

void TizenInputMethodContext::SetInputPanelOptions() {
  FT_ASSERT(imf_context_);
  tizen_core_imf_context_set_input_panel_layout(
      imf_context_, TIZEN_CORE_IMF_INPUT_PANEL_LAYOUT_NORMAL);
  tizen_core_imf_context_set_input_panel_return_key_type(
      imf_context_, TIZEN_CORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT);
}

void TizenInputMethodContext::InputPanelStateChangedCallback(
    tizen_core_imf_context_h ctx,
    int value,
    void* data) {
  auto* self = static_cast<TizenInputMethodContext*>(data);
  tizen_core_imf_input_panel_state_e state =
      static_cast<tizen_core_imf_input_panel_state_e>(value);

  std::string state_str;
  switch (state) {
    case TIZEN_CORE_IMF_INPUT_PANEL_STATE_SHOW:
      state_str = "show";
      break;
    case TIZEN_CORE_IMF_INPUT_PANEL_STATE_HIDE:
      state_str = "hide";
      break;
    case TIZEN_CORE_IMF_INPUT_PANEL_STATE_WILL_SHOW:
      state_str = "will_show";
      break;
    default:
      state_str = "unknown";
      break;
  }

  if (self->on_input_panel_state_changed_) {
    self->on_input_panel_state_changed_(state_str);
  }
}

void TizenInputMethodContext::RegisterInputPanelEventCallback() {
  FT_ASSERT(imf_context_);

  tizen_core_imf_context_add_input_panel_event_callback(
      imf_context_, TIZEN_CORE_IMF_INPUT_PANEL_EVENT_STATE,
      InputPanelStateChangedCallback, this);
}

void TizenInputMethodContext::UnregisterInputPanelEventCallback() {
  FT_ASSERT(imf_context_);

  tizen_core_imf_context_del_input_panel_event_callback(
      imf_context_, TIZEN_CORE_IMF_INPUT_PANEL_EVENT_STATE,
      InputPanelStateChangedCallback);
}

}  // namespace flutter
