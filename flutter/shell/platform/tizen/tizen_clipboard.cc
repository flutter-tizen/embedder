// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_clipboard.h"

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_window.h"
#include "flutter/shell/platform/tizen/tizen_window_ecore_wl2.h"

namespace flutter {

namespace {

constexpr char kMimeTypeTextPlain[] = "text/plain;charset=utf-8";

}  // namespace

TizenClipboard::TizenClipboard(TizenViewBase* view) {
  if (auto* window = dynamic_cast<TizenWindowEcoreWl2*>(view)) {
    auto* ecore_wl2_window =
        static_cast<Ecore_Wl2_Window*>(window->GetNativeHandle());
    display_ = ecore_wl2_window_display_get(ecore_wl2_window);
  } else {
    display_ = ecore_wl2_connected_display_get(NULL);
  }

  send_handler = ecore_event_handler_add(
      ECORE_WL2_EVENT_DATA_SOURCE_SEND,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenClipboard*>(data);
        self->SendData(event);
        return ECORE_CALLBACK_PASS_ON;
      },
      this);
  receive_handler = ecore_event_handler_add(
      ECORE_WL2_EVENT_OFFER_DATA_READY,
      [](void* data, int type, void* event) -> Eina_Bool {
        auto* self = reinterpret_cast<TizenClipboard*>(data);
        self->ReceiveData(event);
        return ECORE_CALLBACK_PASS_ON;
      },
      this);
}

TizenClipboard::~TizenClipboard() {
  ecore_event_handler_del(send_handler);
  ecore_event_handler_del(receive_handler);
}

void TizenClipboard::SendData(void* event) {
  auto* send_event = reinterpret_cast<Ecore_Wl2_Event_Data_Source_Send*>(event);
  if (!send_event->type || strcmp(send_event->type, kMimeTypeTextPlain)) {
    FT_LOG(Error) << "Invaild mime type.";
    return;
  }

  if (send_event->serial != selection_serial_) {
    FT_LOG(Error) << "The serial doesn't match.";
    return;
  }

  write(send_event->fd, data_.c_str(), data_.length());
  close(send_event->fd);
}

void TizenClipboard::ReceiveData(void* event) {
  auto* ready_event =
      reinterpret_cast<Ecore_Wl2_Event_Offer_Data_Ready*>(event);
  if (ready_event->data == nullptr || ready_event->len < 1) {
    FT_LOG(Info) << "No data available.";
    if (on_data_callback_) {
      on_data_callback_("");
      on_data_callback_ = nullptr;
    }
    return;
  }

  if (ready_event->offer != selection_offer_) {
    FT_LOG(Error) << "The offer doesn't match.";
    if (on_data_callback_) {
      on_data_callback_(std::nullopt);
      on_data_callback_ = nullptr;
    }
    return;
  }

  size_t data_length = strlen(ready_event->data);
  size_t buffer_size = ready_event->len;
  std::string content;

  if (data_length < buffer_size) {
    content.append(ready_event->data, data_length);
  } else {
    content.append(ready_event->data, buffer_size);
  }

  if (on_data_callback_) {
    on_data_callback_(content);
    on_data_callback_ = nullptr;
  }
}

void TizenClipboard::SetData(const std::string& data) {
  data_ = data;

  const char* mime_types[3];
  mime_types[0] = kMimeTypeTextPlain;
  // TODO(jsuya): There is an issue where ECORE_WL2_EVENT_DATA_SOURCE_SEND event
  // does not work properly even if ecore_wl2_dnd_selection_set() is called in
  // Tizen 6.5 or lower. Therefore, add empty mimetype for event call from the
  // cbhm module. Since it works normally from Tizen 8.0, this part may be
  // modified in the future.
  mime_types[1] = "";
  mime_types[2] = nullptr;

  Ecore_Wl2_Input* input = ecore_wl2_input_default_input_get(display_);
  selection_serial_ = ecore_wl2_dnd_selection_set(input, mime_types);
  ecore_wl2_display_flush(display_);
}

bool TizenClipboard::GetData(ClipboardCallback on_data_callback) {
  on_data_callback_ = std::move(on_data_callback);

  Ecore_Wl2_Input* input = ecore_wl2_input_default_input_get(display_);
  selection_offer_ = ecore_wl2_dnd_selection_get(input);

  if (!selection_offer_) {
    FT_LOG(Error) << "ecore_wl2_dnd_selection_get() failed.";
    return false;
  }

  ecore_wl2_offer_receive(selection_offer_,
                          const_cast<char*>(kMimeTypeTextPlain));
  return true;
}

bool TizenClipboard::HasStrings() {
  Ecore_Wl2_Input* input = ecore_wl2_input_default_input_get(display_);
  selection_offer_ = ecore_wl2_dnd_selection_get(input);

  if (!selection_offer_) {
    FT_LOG(Error) << "ecore_wl2_dnd_selection_get() failed.";
    return false;
  }

  Eina_Array* available_types = ecore_wl2_offer_mimes_get(selection_offer_);
  unsigned int type_count = eina_array_count(available_types);

  for (unsigned int i = 0; i < type_count; ++i) {
    auto* available_type =
        static_cast<char*>(eina_array_data_get(available_types, i));
    if (!strcmp(kMimeTypeTextPlain, available_type)) {
      return true;
    }
  }
  return false;
}

}  // namespace flutter
