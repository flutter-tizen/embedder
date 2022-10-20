// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"

namespace flutter {

// A test utility class providing the ability to access and alter various
// private fields in an Engine instance.
//
// This simply provides a way to access the normally-private embedder proc
// table, so the lifetime of any changes made to the proc table is that of the
// engine object, not this helper.
class EngineModifier {
 public:
  explicit EngineModifier(FlutterTizenEngine* engine) : engine_(engine) {}

  // Returns the engine's embedder API proc table, allowing for modification.
  //
  // Modifications are to the engine, and will last for the lifetime of the
  // engine unless overwritten again.
  FlutterEngineProcTable& embedder_api() { return engine_->embedder_api_; }

 private:
  FlutterTizenEngine* engine_;
};

}  // namespace flutter
