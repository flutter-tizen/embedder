// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "input_panel_channel.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/event_stream_handler_functions.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_method_codec.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_input_method_context.h"

namespace {

constexpr char kChannelName[] = "tizen/internal/inputpanel";

}  // namespace

namespace flutter {

InputPanelChannel::InputPanelChannel(BinaryMessenger* messenger,
                                     TizenInputMethodContext* imf_context)
    : imf_context_(imf_context) {
  event_channel_ = std::make_unique<EventChannel<EncodableValue>>(
      messenger, kChannelName, &StandardMethodCodec::GetInstance());
  auto event_channel_handler = std::make_unique<StreamHandlerFunctions<>>(
      [this](const EncodableValue* arguments,
             std::unique_ptr<EventSink<>>&& events)
          -> std::unique_ptr<StreamHandlerError<>> {
        event_sink_ = std::move(events);
        imf_context_->SetOnInputPanelStateChanged(
            [this](const std::string& state) {
              SendInputPanelStateEvent(state);
            });
        return nullptr;
      },
      [this](const EncodableValue* arguments)
          -> std::unique_ptr<StreamHandlerError<>> {
        imf_context_->SetOnInputPanelStateChanged(nullptr);
        event_sink_.reset();
        return nullptr;
      });

  event_channel_->SetStreamHandler(std::move(event_channel_handler));
}

InputPanelChannel::~InputPanelChannel() {
  imf_context_->SetOnInputPanelStateChanged(nullptr);
}

void InputPanelChannel::SendInputPanelStateEvent(const std::string& state) {
  if (!event_sink_) {
    return;
  }

  EncodableMap event;
  event[EncodableValue("state")] = EncodableValue(state);
  event_sink_->Success(EncodableValue(event));
}

}  // namespace flutter
