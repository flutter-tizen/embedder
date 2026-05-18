// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_CLIPBOARD_H_
#define EMBEDDER_TIZEN_CLIPBOARD_H_

#ifdef USE_TCORE_WL
#include <tizen_core_wl.h>
#else
#define EFL_BETA_API_SUPPORT
#include <Ecore_Wl2.h>
#endif

#include <functional>
#include <optional>
#include <string>

#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

class TizenClipboard {
 public:
  using ClipboardCallback =
      std::function<void(std::optional<std::string> data)>;

  TizenClipboard(TizenViewBase* view);
  virtual ~TizenClipboard();

  void SetData(const std::string& data);
  bool GetData(ClipboardCallback on_data_callback);
  bool HasStrings();

 private:
  void SendData(void* event);
  void ReceiveData(void* event);

  std::string data_;
  ClipboardCallback on_data_callback_;
  uint32_t selection_serial_ = 0;
#ifdef USE_TCORE_WL
  tizen_core_wl_data_offer_h selection_offer_ = nullptr;
  tizen_core_wl_display_h display_ = nullptr;
  tizen_core_event_h tcore_wl_event_ = nullptr;
  tizen_core_wl_event_listener_h send_listener_ = nullptr;
  tizen_core_wl_event_listener_h receive_listener_ = nullptr;
#else
  Ecore_Wl2_Offer* selection_offer_ = nullptr;
  Ecore_Wl2_Display* display_ = nullptr;
  Ecore_Event_Handler* send_handler = nullptr;
  Ecore_Event_Handler* receive_handler = nullptr;
#endif
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_CLIPBOARD_H_
