// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_TCORE_WL_H_
#define EMBEDDER_TIZEN_WINDOW_TCORE_WL_H_

#include <tizen_core_wl.h>
#include <tizen_core_wl_internal.h>
#include <tizen-extension-client-protocol.h>

#include <cstdint>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

class TizenWindowTcoreWl : public TizenWindow {
 public:
  TizenWindowTcoreWl(TizenGeometry geometry,
                      bool transparent,
                      bool focusable,
                      bool top_level,
                      bool pointing_device_support,
                      bool floating_menu_support,
                      void* window_handle,
                      bool is_vulkan);

  ~TizenWindowTcoreWl();

  TizenGeometry GetGeometry() override;

  bool SetGeometry(TizenGeometry geometry) override;

  TizenGeometry GetScreenGeometry() override;

  void* GetRenderTarget() override;

  void* GetRenderTargetDisplay() override { return wl2_display_; }

  void* GetNativeHandle() override { return tcore_wl_window_; }

  int32_t GetRotation() override;

  int32_t GetDpi() override;

  uintptr_t GetWindowId() override;

  uint32_t GetResourceId() override;

  void SetPreferredOrientations(const std::vector<int>& rotations) override;

  void BindKeys(const std::vector<std::string>& keys) override;

  void Show() override;

  void UpdateFlutterCursor(const std::string& kind) override;

  void ActivateWindow() override;

  void RaiseWindow() override;

  void LowerWindow() override;

 private:
  bool CreateWindow(void* window_handle);

  void DestroyWindow();

  void SetWindowOptions();

  void EnableCursor();

#ifdef TV_PROFILE
  void SetPointingDeviceSupport();

  void SetFloatingMenuSupport();

  void ShowUnsupportedToast();
#endif

  void RegisterEventHandlers();

  void UnregisterEventHandlers();

  void SetNotificationLevel(int level);

  void PrepareInputMethod();

  tizen_core_wl_display_h tcore_wl_display_ = nullptr;
  tizen_core_wl_window_h tcore_wl_window_ = nullptr;
  tizen_core_wl_egl_window_h tcore_wl_egl_window_ = nullptr;
  tizen_core_event_h tcore_wl_event_ = nullptr;
  wl_display* wl2_display_ = nullptr;
  wl_surface* wl2_surface_ = nullptr;
  std::vector<tizen_core_wl_event_listener_h> tcore_event_listeners_;
  uint32_t resource_id_ = 0;

#ifdef TV_PROFILE
  bool pointing_device_support_ = true;
  bool floating_menu_support_ = true;
  bool show_unsupported_toast_ = false;
#endif
  bool is_vulkan_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_WINDOW_TCORE_WL_H_
