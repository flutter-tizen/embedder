// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_clipboard.h"

#include <unistd.h>

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

#ifdef USE_TCORE_WL
#include "flutter/shell/platform/tizen/tizen_window_tcore_wl.h"
#else
#include "flutter/shell/platform/tizen/tizen_window_ecore_wl2.h"
#endif

namespace flutter {

namespace {

constexpr char kMimeTypeTextPlain[] = "text/plain;charset=utf-8";

}  // namespace

#ifdef USE_TCORE_WL

TizenClipboard::TizenClipboard(TizenViewBase* view) {
  if (auto* window = dynamic_cast<TizenWindowTcoreWl*>(view)) {
    tizen_core_wl_window_h tcore_window =
        static_cast<tizen_core_wl_window_h>(window->GetNativeHandle());
    tizen_core_wl_window_get_display(tcore_window, &display_);
  } else {
    // TODO(jsuya): tizen_core_wl_get_connected_display() will be deprecated.
    tizen_core_wl_get_connected_display(nullptr, &display_);
  }

  tizen_core_wl_display_get_event(display_, &tcore_wl_event_);

  tizen_core_wl_event_add_listener(
      tcore_wl_event_, TIZEN_CORE_WL_EVENT_DATA_SOURCE_SEND,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenClipboard*>(data);
        self->SendData(event);
      },
      this, &send_listener_);

  tizen_core_wl_event_add_listener(
      tcore_wl_event_, TIZEN_CORE_WL_EVENT_DATA_READY,
      [](void* event, tizen_core_wl_event_type_e type, void* data) {
        auto* self = static_cast<TizenClipboard*>(data);
        self->ReceiveData(event);
      },
      this, &receive_listener_);
}

TizenClipboard::~TizenClipboard() {
  on_data_callback_ = nullptr;

  if (send_listener_) {
    tizen_core_wl_event_remove_listener(tcore_wl_event_, send_listener_);
  }
  if (receive_listener_) {
    tizen_core_wl_event_remove_listener(tcore_wl_event_, receive_listener_);
  }
}

void TizenClipboard::SendData(void* event) {
  if (!event) {
    return;
  }
  tizen_core_wl_event_data_source_send_h send_event =
      static_cast<tizen_core_wl_event_data_source_send_h>(event);

  char* mime_type = nullptr;
  tizen_core_wl_event_data_source_send_get_type(send_event, &mime_type);
  int fd = -1;
  tizen_core_wl_event_data_source_send_get_fd(send_event, &fd);

  // Get serial from base event.
  tizen_core_wl_event_data_source_base_h base_event =
      static_cast<tizen_core_wl_event_data_source_base_h>(event);
  unsigned int serial = 0;
  tizen_core_wl_event_data_source_base_get_serial(base_event, &serial);

  if (!mime_type ||
      (strlen(mime_type) != 0 && strcmp(mime_type, kMimeTypeTextPlain))) {
    FT_LOG(Error) << "Invaild mime type(" << (mime_type ? mime_type : "null")
                  << ").";
    if (fd >= 0) {
      close(fd);
    }
    return;
  }

  if (serial != selection_serial_) {
    FT_LOG(Error) << "The serial doesn't match.";
    if (fd >= 0) {
      close(fd);
    }
    return;
  }

  write(fd, data_.c_str(), data_.length());
  close(fd);
}

void TizenClipboard::ReceiveData(void* event) {
  if (!event) {
    return;
  }
  tizen_core_wl_event_data_ready_h ready_event =
      static_cast<tizen_core_wl_event_data_ready_h>(event);

  void* raw_data = nullptr;
  tizen_core_wl_event_data_ready_get_data(ready_event, &raw_data);
  const char* data = static_cast<const char*>(raw_data);
  int len = 0;
  tizen_core_wl_event_data_ready_get_len(ready_event, &len);

  if (data == nullptr || len < 1) {
    FT_LOG(Info) << "No data available.";
    if (on_data_callback_) {
      on_data_callback_("");
      on_data_callback_ = nullptr;
    }
    return;
  }

  tizen_core_wl_data_offer_h offer = nullptr;
  tizen_core_wl_event_data_ready_get_offer(ready_event, &offer);

  if (offer != selection_offer_) {
    FT_LOG(Error) << "The offer doesn't match.";
    if (on_data_callback_) {
      on_data_callback_(std::nullopt);
      on_data_callback_ = nullptr;
    }
    return;
  }

  size_t data_length = strlen(data);
  size_t buffer_size = len;
  std::string content;

  if (data_length < buffer_size) {
    content.append(data, data_length);
  } else {
    content.append(data, buffer_size);
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

  tizen_core_wl_seat_h default_seat = nullptr;
  tizen_core_wl_display_get_default_seat(display_, &default_seat);
  if (default_seat) {
    tizen_core_wl_data_h data_handle = nullptr;
    tizen_core_wl_seat_get_data(default_seat, &data_handle);
    if (data_handle) {
      tizen_core_wl_data_set_selection(data_handle, mime_types,
                                       &selection_serial_);
    }
  }
  tizen_core_wl_display_flush(display_);
}

bool TizenClipboard::GetData(ClipboardCallback on_data_callback) {
  on_data_callback_ = std::move(on_data_callback);

  tizen_core_wl_seat_h default_seat = nullptr;
  tizen_core_wl_display_get_default_seat(display_, &default_seat);
  if (!default_seat) {
    if (on_data_callback_) {
      on_data_callback_ = nullptr;
    }
    return false;
  }

  tizen_core_wl_data_h data_handle = nullptr;
  tizen_core_wl_seat_get_data(default_seat, &data_handle);
  if (!data_handle) {
    if (on_data_callback_) {
      on_data_callback_ = nullptr;
    }
    return false;
  }

  tizen_core_wl_data_offer_h offer = nullptr;
  tizen_core_wl_data_get_selection(data_handle, &offer);
  selection_offer_ = offer;
  if (!selection_offer_) {
    FT_LOG(Error) << "tizen_core_wl_data_get_selection() failed.";
    if (on_data_callback_) {
      on_data_callback_ = nullptr;
    }
    return false;
  }

  tizen_core_wl_data_receive(selection_offer_,
                             const_cast<char*>(kMimeTypeTextPlain));
  return true;
}

bool TizenClipboard::HasStrings() {
  tizen_core_wl_seat_h default_seat = nullptr;
  tizen_core_wl_display_get_default_seat(display_, &default_seat);
  if (!default_seat) {
    return false;
  }

  tizen_core_wl_data_h data_handle = nullptr;
  tizen_core_wl_seat_get_data(default_seat, &data_handle);
  if (!data_handle) {
    return false;
  }

  tizen_core_wl_data_offer_h offer = nullptr;
  tizen_core_wl_data_get_selection(data_handle, &offer);
  selection_offer_ = offer;
  if (!selection_offer_) {
    return false;
  }

  char** mimes = nullptr;
  int mime_count = 0;
  tizen_core_wl_data_get_mimes(selection_offer_, &mimes, &mime_count);

  if (!mimes) {
    return false;
  }

  bool found = false;
  for (int i = 0; i < mime_count; ++i) {
    if (mimes[i] && !strcmp(kMimeTypeTextPlain, mimes[i])) {
      found = true;
      break;
    }
  }
  free(mimes);
  return found;
}

#else  // USE_TCORE_WL

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
  on_data_callback_ = nullptr;

  ecore_event_handler_del(send_handler);
  ecore_event_handler_del(receive_handler);
}

void TizenClipboard::SendData(void* event) {
  if (!event) {
    return;
  }
  auto* send_event = reinterpret_cast<Ecore_Wl2_Event_Data_Source_Send*>(event);

  // TODO(jsuya): If the type of Ecore_Wl2_Event_Data_Source_Send is empty, it
  // is assumed to be "text/plain".
  if (!send_event->type || (strlen(send_event->type) != 0 &&
                            strcmp(send_event->type, kMimeTypeTextPlain))) {
    FT_LOG(Error) << "Invaild mime type("
                  << (send_event->type ? send_event->type : "null") << ").";
    if (send_event->fd >= 0) {
      close(send_event->fd);
    }
    return;
  }

  if (send_event->serial != selection_serial_) {
    FT_LOG(Error) << "The serial doesn't match.";
    if (send_event->fd >= 0) {
      close(send_event->fd);
    }
    return;
  }

  write(send_event->fd, data_.c_str(), data_.length());
  close(send_event->fd);
}

void TizenClipboard::ReceiveData(void* event) {
  if (!event) {
    return;
  }
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

    if (on_data_callback_) {
      on_data_callback_ = nullptr;
    }
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

#endif  // USE_TCORE_WL

}  // namespace flutter
