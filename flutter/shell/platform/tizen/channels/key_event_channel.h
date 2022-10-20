// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_KEY_EVENT_CHANNEL_H_
#define EMBEDDER_KEY_EVENT_CHANNEL_H_

#include <functional>
#include <map>
#include <memory>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/basic_message_channel.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "rapidjson/document.h"

namespace flutter {

class KeyEventChannel {
 public:
  using SendEventHandler = std::function<void(const FlutterKeyEvent& event,
                                              FlutterKeyEventCallback callback,
                                              void* user_data)>;

  explicit KeyEventChannel(BinaryMessenger* messenger,
                           SendEventHandler send_event);
  virtual ~KeyEventChannel();

  void SendKey(const char* key,
               const char* string,
               const char* compose,
               uint32_t modifiers,
               uint32_t scan_code,
               bool is_down,
               std::function<void(bool)> callback);

 private:
  std::unique_ptr<BasicMessageChannel<rapidjson::Document>> channel_;
  SendEventHandler send_event_;

  struct PendingEvent {
    // The number of handlers that haven't replied.
    size_t unreplied;
    // Whether any replied handlers reported true (handled).
    bool any_handled;
    // Where to report the handlers' result to.
    std::function<void(bool)> callback;
  };

  // Key events that have been sent to the framework but have not yet received
  // a response, indexed by sequence IDs.
  std::map<uint64_t, std::unique_ptr<PendingEvent>> pending_events_;

  // A self-incrementing integer used as the ID for the next entry for
  // |pending_events_|.
  uint64_t last_sequence_id_ = 0;

  // A map from physical keys to logical keys, each entry indicating a pressed
  // key.
  std::map<uint64_t, uint64_t> pressing_records_;

  void SendEmbedderEvent(const char* key,
                         const char* string,
                         const char* compose,
                         uint32_t modifiers,
                         uint32_t scan_code,
                         bool is_down,
                         uint64_t sequence_id);

  void SendChannelEvent(const char* key,
                        const char* string,
                        const char* compose,
                        uint32_t modifiers,
                        uint32_t scan_code,
                        bool is_down,
                        uint64_t sequence_id);

  void ResolvePendingEvent(uint64_t sequence_id, bool handled);
};

}  // namespace flutter

#endif  //  EMBEDDER_KEY_EVENT_CHANNEL_H_
