// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ENCODABLE_VALUE_HOLDER_H_
#define EMBEDDER_ENCODABLE_VALUE_HOLDER_H_

#include "flutter/shell/platform/common/client_wrapper/include/flutter/encodable_value.h"

namespace flutter {

template <typename T>
struct EncodableValueHolder {
  EncodableValueHolder(const EncodableMap* encodable_map,
                       const std::string& key) {
    auto iter = encodable_map->find(EncodableValue(key));
    if (iter != encodable_map->end() && !iter->second.IsNull()) {
      value = std::get_if<T>(&iter->second);
    }
  }

  ~EncodableValueHolder() {}

  const T& operator*() { return *value; }

  const T* operator->() { return value; }

  explicit operator bool() const { return value; }

  const T* value = nullptr;
};

}  // namespace flutter

#endif  // EMBEDDER_ENCODABLE_VALUE_HOLDER_H_
