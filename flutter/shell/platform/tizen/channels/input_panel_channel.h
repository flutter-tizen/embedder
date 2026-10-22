// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_INPUT_PANEL_CHANNEL_H_
#define EMBEDDER_INPUT_PANEL_CHANNEL_H_

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_channel.h"

namespace flutter {

class TizenInputMethodContext;

class InputPanelChannel {
 public:
  explicit InputPanelChannel(BinaryMessenger* messenger,
                             TizenInputMethodContext* imf_context);
  virtual ~InputPanelChannel();

 private:
  void SendInputPanelStateEvent(const std::string& state);

  std::unique_ptr<EventChannel<EncodableValue>> event_channel_;
  std::unique_ptr<EventSink<EncodableValue>> event_sink_;
  TizenInputMethodContext* imf_context_;
};

}  // namespace flutter

#endif  // EMBEDDER_INPUT_PANEL_CHANNEL_H_
