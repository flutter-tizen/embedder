// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_VIEW_H_
#define EMBEDDER_FLUTTER_TIZEN_VIEW_H_

#include <memory>
#include <string>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/channels/platform_channel.h"
#include "flutter/shell/platform/tizen/channels/text_input_channel.h"
#include "flutter/shell/platform/tizen/channels/window_channel.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"
#include "flutter/shell/platform/tizen/tizen_view_event_handler_delegate.h"

namespace flutter {

class FlutterTizenView : public TizenViewEventHandlerDelegate {
 public:
  FlutterTizenView(std::unique_ptr<TizenViewBase> tizen_view);

  virtual ~FlutterTizenView();

  // Configures the window instance with an instance of a running Flutter
  // engine.
  void SetEngine(std::unique_ptr<FlutterTizenEngine> engine);

  FlutterTizenEngine* engine() { return engine_.get(); }

  TizenViewBase* tizen_view() { return tizen_view_.get(); }

  // Creates rendering surface for Flutter engine to draw into.
  // Should be called before calling FlutterEngineRun using this view.
  void CreateRenderSurface(FlutterDesktopRendererType renderer_type);

  // Destroys current rendering surface if one has been allocated.
  void DestroyRenderSurface();

  void Resize(int32_t width, int32_t height);

  // Callbacks for clearing context, settings context and swapping buffers,
  // these are typically called on an engine-controlled (non-platform) thread.
  bool OnMakeCurrent();
  bool OnClearCurrent();
  bool OnMakeResourceCurrent();
  bool OnPresent();

  uint32_t OnGetFBO();

  void* OnProcResolver(const char* name);

  void OnResize(int32_t left,
                int32_t top,
                int32_t width,
                int32_t height) override;

  void OnRotate(int32_t degree) override;

  void OnPointerMove(double x,
                     double y,
                     size_t timestamp,
                     FlutterPointerDeviceKind device_kind,
                     int32_t device_id) override;

  void OnPointerDown(double x,
                     double y,
                     size_t timestamp,
                     FlutterPointerDeviceKind device_kind,
                     int32_t device_id) override;

  void OnPointerUp(double x,
                   double y,
                   size_t timestamp,
                   FlutterPointerDeviceKind device_kind,
                   int32_t device_id) override;

  void OnScroll(double x,
                double y,
                double delta_x,
                double delta_y,
                int scroll_offset_multiplier,
                size_t timestamp,
                FlutterPointerDeviceKind device_kind,
                int32_t device_id) override;

  void OnKey(const char* key,
             const char* string,
             const char* compose,
             uint32_t modifiers,
             uint32_t scan_code,
             bool is_down) override;

  void OnComposeBegin() override;

  void OnComposeChange(const std::string& str, int cursor_pos) override;

  void OnComposeEnd() override;

  void OnCommit(const std::string& str) override;

  FlutterTransformation GetFlutterTransformation() {
    return flutter_transformation_;
  }

  void SendInitialGeometry();

  TextInputChannel* text_input_channel() { return text_input_channel_.get(); }

 private:
  // Sends a window metrics update to the Flutter engine using current window
  // dimensions in physical.
  void SendWindowMetrics(int32_t left,
                         int32_t top,
                         int32_t width,
                         int32_t height,
                         double pixel_ratio);

  // Reports pointer event to Flutter engine.
  void SendFlutterPointerEvent(FlutterPointerPhase phase,
                               double x,
                               double y,
                               double delta_x,
                               double delta_y,
                               size_t timestamp,
                               FlutterPointerDeviceKind device_kind,
                               int device_id);

  // The engine associated with this view.
  std::unique_ptr<FlutterTizenEngine> engine_;

  // The platform view associated with this Flutter view.
  std::unique_ptr<TizenViewBase> tizen_view_;

  // The plugin registrar managing internal plugins.
  std::unique_ptr<PluginRegistrar> internal_plugin_registrar_;

  // A plugin that implements the Tizen window channel.
  std::unique_ptr<WindowChannel> window_channel_;

  // A plugin that implements the Flutter platform channel.
  std::unique_ptr<PlatformChannel> platform_channel_;

  // A plugin that implements the Flutter textinput channel.
  std::unique_ptr<TextInputChannel> text_input_channel_;

  // The current view rotation degree.
  int32_t rotation_degree_ = 0;

  // The current pointer state to distinguish move or hover event.
  bool pointer_state_ = false;

  // The current view transformation.
  FlutterTransformation flutter_transformation_ = {1.0, 0.0, 0.0, 0.0, 1.0,
                                                   0.0, 0.0, 0.0, 1.0};
};

}  // namespace flutter

#endif  // EMBEDDER_FLUTTER_TIZEN_VIEW_H_
