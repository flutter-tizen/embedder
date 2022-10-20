// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_STRING_CODEC_H_
#define EMBEDDER_STRING_CODEC_H_

#include <memory>
#include <string>
#include <variant>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/message_codec.h"

namespace flutter {

// A string message encoding/decoding mechanism for communications to/from the
// Flutter engine via message channels.
class StringCodec : public MessageCodec<EncodableValue> {
 public:
  ~StringCodec() = default;

  // Returns an instance of the codec.
  static const StringCodec& GetInstance() {
    static StringCodec sInstance;
    return sInstance;
  }

  // Prevent copying.
  StringCodec(StringCodec const&) = delete;
  StringCodec& operator=(StringCodec const&) = delete;

 protected:
  // |flutter::MessageCodec|
  std::unique_ptr<EncodableValue> DecodeMessageInternal(
      const uint8_t* binary_message,
      const size_t message_size) const override {
    return std::make_unique<EncodableValue>(std::string(
        reinterpret_cast<const char*>(binary_message), message_size));
  }

  // |flutter::MessageCodec|
  std::unique_ptr<std::vector<uint8_t>> EncodeMessageInternal(
      const EncodableValue& message) const override {
    auto string_value = std::get<std::string>(message);
    return std::make_unique<std::vector<uint8_t>>(string_value.begin(),
                                                  string_value.end());
  }

 private:
  // Instances should be obtained via GetInstance.
  explicit StringCodec() = default;
};

}  // namespace flutter

#endif  // EMBEDDER_STRING_CODEC_H_
