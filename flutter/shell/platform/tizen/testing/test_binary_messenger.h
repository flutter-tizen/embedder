// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TESTING_TEST_BINARY_MESSENGER_H_
#define EMBEDDER_TESTING_TEST_BINARY_MESSENGER_H_

#include <cassert>
#include <functional>
#include <map>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/binary_messenger.h"

namespace flutter {

// A trivial BinaryMessenger implementation for use in tests.
class TestBinaryMessenger : public BinaryMessenger {
 public:
  using SendHandler = std::function<void(const std::string& channel,
                                         const uint8_t* message,
                                         size_t message_size,
                                         BinaryReply reply)>;

  // Creates a new messenge that forwards all calls to |send_handler|.
  explicit TestBinaryMessenger(SendHandler send_handler = nullptr)
      : send_handler_(std::move(send_handler)) {}

  virtual ~TestBinaryMessenger() = default;

  // Prevent copying.
  TestBinaryMessenger(TestBinaryMessenger const&) = delete;
  TestBinaryMessenger& operator=(TestBinaryMessenger const&) = delete;

  // Simulates a message from the engine on the given channel.
  //
  // Returns false if no handler is registered on that channel.
  bool SimulateEngineMessage(const std::string& channel,
                             const uint8_t* message,
                             size_t message_size,
                             BinaryReply reply) {
    auto handler = registered_handlers_.find(channel);
    if (handler == registered_handlers_.end()) {
      return false;
    }
    (handler->second)(message, message_size, reply);
    return true;
  }

  // |flutter::BinaryMessenger|
  void Send(const std::string& channel,
            const uint8_t* message,
            size_t message_size,
            BinaryReply reply) const override {
    // If something under test sends a message, the test should be handling it.
    assert(send_handler_);
    send_handler_(channel, message, message_size, reply);
  }

  // |flutter::BinaryMessenger|
  void SetMessageHandler(const std::string& channel,
                         BinaryMessageHandler handler) override {
    if (handler) {
      registered_handlers_[channel] = handler;
    } else {
      registered_handlers_.erase(channel);
    }
  }

 private:
  // Handler to call for SendMessage.
  SendHandler send_handler_;

  // Mapping of channel name to registered handlers.
  std::map<std::string, BinaryMessageHandler> registered_handlers_;
};

}  // namespace flutter

#endif  // EMBEDDER_TESTING_TEST_BINARY_MESSENGER_H_
