// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_ELEMENTARY_H_
#define EMBEDDER_TIZEN_WINDOW_ELEMENTARY_H_

#include <Elementary.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "flutter/shell/platform/tizen/public/flutter_tizen.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

class TizenWindowElementary : public TizenWindow {
 public:
  TizenWindowElementary(TizenGeometry geometry,
                        bool transparent,
                        bool focusable,
                        bool top_level);

  ~TizenWindowElementary();

  TizenGeometry GetGeometry() override;

  bool SetGeometry(TizenGeometry geometry) override;

  TizenGeometry GetScreenGeometry() override;

  void* GetRenderTarget() override { return image_; }

  void* GetRenderTargetDisplay() override { return nullptr; }

  void* GetNativeHandle() override { return elm_win_; }

  int32_t GetRotation() override;

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  uint32_t GetResourceId() override;

  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  void BindKeys(const std::vector<std::string>& keys) override;

  void Show() override;

  void UpdateFlutterCursor(const std::string& kind) override;

 private:
  bool CreateWindow();

  void DestroyWindow();

  void SetWindowOptions();

  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void PrepareInputMethod();

  Evas_Object* elm_win_ = nullptr;
  Evas_Object* image_ = nullptr;

  Evas_Smart_Cb rotation_changed_callback_ = nullptr;
  std::unordered_map<Evas_Callback_Type, Evas_Object_Event_Cb>
      evas_object_callbacks_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_ELEMENTARY_H_
