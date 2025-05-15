// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_DISPLAY_MONITOR_H_
#define EMBEDDER_FLUTTER_TIZEN_DISPLAY_MONITOR_H_

namespace flutter {

class FlutterTizenEngine;
class FlutterTizenDisplayMonitor {
 public:
  FlutterTizenDisplayMonitor(FlutterTizenEngine* engine);
  ~FlutterTizenDisplayMonitor();

  // Updates the display information and notifies the engine
  void UpdateDisplays();

 private:
  FlutterTizenEngine* engine_;
};
}  // namespace flutter

#endif  // EMBEDDER_FLUTTER_TIZEN_DISPLAY_MONITOR_H_
