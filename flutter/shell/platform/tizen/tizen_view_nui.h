// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_VIEW_NUI_H_
#define EMBEDDER_TIZEN_VIEW_NUI_H_

#include <dali-toolkit/public-api/controls/image-view/image-view.h>
#include <dali/devel-api/adaptor-framework/event-thread-callback.h>
#include <dali/devel-api/adaptor-framework/native-image-source-queue.h>
#include <dali/devel-api/common/stage.h>

#include <memory>

#include "flutter/shell/platform/tizen/tizen_view.h"

namespace flutter {

class TizenViewNui : public TizenView {
 public:
  TizenViewNui(int32_t width,
               int32_t height,
               Dali::Toolkit::ImageView* image_view,
               Dali::NativeImageSourceQueuePtr native_image_queue,
               int32_t default_window_id);

  ~TizenViewNui();

  TizenGeometry GetGeometry() override;

  bool SetGeometry(TizenGeometry geometry) override;

  void* GetRenderTarget() override { return native_image_queue_.Get(); }

  void* GetNativeHandle() override { return image_view_; }

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  uint32_t GetResourceId() override;

  void Show() override;

  void RequestRendering();

  void OnKey(const char* device_name,
             uint32_t device_class,
             uint32_t device_subclass,
             const char* key,
             const char* string,
             const char* compose,
             uint32_t modifiers,
             uint32_t scan_code,
             size_t timestamp,
             bool is_down);

  void UpdateFlutterCursor(const std::string& kind) override;

 private:
  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void PrepareInputMethod();

  void RenderOnce();

  Dali::Toolkit::ImageView* image_view_ = nullptr;
  Dali::NativeImageSourceQueuePtr native_image_queue_;
  int32_t default_window_id_;
  std::unique_ptr<Dali::EventThreadCallback> rendering_callback_;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_VIEW_NUI_H_
