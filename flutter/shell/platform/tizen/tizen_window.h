// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_H_
#define EMBEDDER_TIZEN_WINDOW_H_

#include <cstdint>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_view_base.h"

namespace flutter {

class FlutterTizenView;

class TizenWindow : public TizenViewBase {
 public:
  TizenWindow() = default;
  virtual ~TizenWindow() = default;

  virtual int32_t GetRotation() = 0;

  virtual void SetPreferredOrientations(const std::vector<int>& rotations) = 0;

  virtual void* GetRenderTargetDisplay() = 0;

  // Returns the geometry of the display screen.
  virtual TizenGeometry GetScreenGeometry() = 0;

  virtual void BindKeys(const std::vector<std::string>& keys) = 0;

  TizenViewType GetType() override { return TizenViewType::kWindow; };

 protected:
  explicit TizenWindow(TizenGeometry geometry,
                       bool transparent,
                       bool focusable,
                       bool top_level)
      : initial_geometry_(geometry),
        transparent_(transparent),
        focusable_(focusable),
        top_level_(top_level) {}

  TizenGeometry initial_geometry_ = {0, 0, 0, 0};
  bool transparent_ = false;
  bool focusable_ = false;
  bool top_level_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_H_
