// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_H_
#define EMBEDDER_TIZEN_VIEW_H_

#include <cstdint>

#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

class FlutterTizenView;

class TizenView : public TizenViewBase {
 public:
  TizenView() = default;
  virtual ~TizenView() = default;

  TizenViewType GetType() override { return TizenViewType::kView; };

  bool focused() { return focused_; };

  void SetFocus(bool focused) { focused_ = focused; };

 protected:
  explicit TizenView(int32_t width, int32_t height)
      : initial_width_(width), initial_height_(height) {}

  int32_t initial_width_ = 0;
  int32_t initial_height_ = 0;
  bool focused_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_H_
