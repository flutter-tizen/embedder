// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_view.h"

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_view.h"
#ifdef NUI_SUPPORT
#include "flutter/shell/platform/tizen/tizen_view_nui.h"
#endif
#include "flutter/shell/platform/tizen/tizen_renderer_egl.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace {

#if defined(MOBILE_PROFILE)
constexpr double kProfileFactor = 0.7;
#elif defined(TV_PROFILE)
constexpr double kProfileFactor = 2.0;
#else
constexpr double kProfileFactor = 1.0;
#endif

constexpr char kSysMenuKey[] = "XF86SysMenu";
constexpr char kBackKey[] = "XF86Back";
constexpr char kExitKey[] = "XF86Exit";

// Keys that should always be handled by the app first but not by the system.
const std::vector<std::string> kBindableSystemKeys = {
    "XF86Menu",           "XF86Back",         "XF86AudioPlay",
    "XF86AudioPause",     "XF86AudioStop",    "XF86AudioNext",
    "XF86AudioPrev",      "XF86AudioRewind",  "XF86AudioForward",
    "XF86AudioPlayPause", "XF86AudioRecord",  "XF86LowerChannel",
    "XF86RaiseChannel",   "XF86ChannelList",  "XF86PreviousChannel",
    "XF86SimpleMenu",     "XF86History",      "XF86Favorites",
    "XF86Info",           "XF86Red",          "XF86Green",
    "XF86Yellow",         "XF86Blue",         "XF86Subtitle",
    "XF86PlayBack",       "XF86ChannelGuide", "XF86Caption",
    "XF86Exit",
};

// The multiplier is taken from the Chromium source
// (ui/events/x/events_x_utils.cc).
constexpr int32_t kScrollOffsetMultiplier = 53;

double ComputePixelRatio(flutter::TizenViewBase* view) {
  // The scale factor is computed based on the display DPI and the current
  // profile. A fixed DPI value (72) is used on TVs. See:
  // https://docs.tizen.org/application/native/guides/ui/efl/multiple-screens
#ifdef TV_PROFILE
  double dpi = 72.0;
#else
  double dpi = static_cast<double>(view->GetDpi());
#endif
  double scale_factor = dpi / 90.0 * kProfileFactor;
  return std::max(scale_factor, 1.0);
}

}  // namespace

namespace flutter {

FlutterTizenView::FlutterTizenView(FlutterViewId view_id,
                                   std::unique_ptr<TizenViewBase> tizen_view)
    : view_id_(view_id), tizen_view_(std::move(tizen_view)) {
  tizen_view_->SetView(this);

  if (auto* window = dynamic_cast<TizenWindow*>(tizen_view_.get())) {
    window->BindKeys(kBindableSystemKeys);
  }
}

FlutterTizenView::~FlutterTizenView() {
  if (engine_) {
    if (platform_view_channel_) {
      platform_view_channel_->Dispose();
    }
    engine_->StopEngine();
  }
  DestroyRenderSurface();
}

void FlutterTizenView::SetEngine(std::unique_ptr<FlutterTizenEngine> engine) {
  engine_ = std::move(engine);
  engine_->SetView(this);

  internal_plugin_registrar_ =
      std::make_unique<PluginRegistrar>(engine_->plugin_registrar());

  // Set up window dependent channels.
  BinaryMessenger* messenger = internal_plugin_registrar_->messenger();

  auto* window = dynamic_cast<TizenWindow*>(tizen_view_.get());
  window_channel_ = std::make_unique<WindowChannel>(messenger, window);

  platform_channel_ =
      std::make_unique<PlatformChannel>(messenger, tizen_view_.get());

  double pixel_ratio = ComputePixelRatio(tizen_view_.get());
  platform_view_channel_ =
      std::make_unique<PlatformViewChannel>(messenger, pixel_ratio);
  mouse_cursor_channel_ =
      std::make_unique<MouseCursorChannel>(messenger, tizen_view_.get());
  text_input_channel_ = std::make_unique<TextInputChannel>(
      internal_plugin_registrar_->messenger(),
      tizen_view_->input_method_context());

  input_device_channel_ = std::make_unique<InputDeviceChannel>(messenger);
}

void FlutterTizenView::CreateRenderSurface(
    FlutterDesktopRendererType renderer_type) {
  if (engine_) {
    engine_->CreateRenderer(renderer_type);
  }

  if (engine_ && engine_->renderer()) {
    TizenGeometry geometry = tizen_view_->GetGeometry();
    if (dynamic_cast<TizenWindow*>(tizen_view_.get())) {
      auto* window = dynamic_cast<TizenWindow*>(tizen_view_.get());
      engine_->renderer()->CreateSurface(window->GetRenderTarget(),
                                         window->GetRenderTargetDisplay(),
                                         geometry.width, geometry.height);
    } else {
      auto* tizen_view = dynamic_cast<TizenView*>(tizen_view_.get());
      engine_->renderer()->CreateSurface(tizen_view->GetRenderTarget(), nullptr,
                                         geometry.width, geometry.height);
    }
  }
}

void FlutterTizenView::DestroyRenderSurface() {
  if (engine_ && engine_->renderer()) {
    engine_->renderer()->DestroySurface();
  }
}

void FlutterTizenView::Resize(int32_t width, int32_t height) {
  TizenGeometry geometry = tizen_view_->GetGeometry();
  geometry.width = width;
  geometry.height = height;
  tizen_view_->SetGeometry(geometry);
}

bool FlutterTizenView::OnMakeCurrent() {
  return engine_->renderer()->OnMakeCurrent();
}

bool FlutterTizenView::OnClearCurrent() {
  return engine_->renderer()->OnClearCurrent();
}

bool FlutterTizenView::OnMakeResourceCurrent() {
  return engine_->renderer()->OnMakeResourceCurrent();
}

bool FlutterTizenView::OnPresent() {
  bool result = engine_->renderer()->OnPresent();
#ifdef NUI_SUPPORT
  if (auto* nui_view =
          dynamic_cast<flutter::TizenViewNui*>(tizen_view_.get())) {
    nui_view->RequestRendering();
  }
#endif
  return result;
}

uint32_t FlutterTizenView::OnGetFBO() {
  return engine_->renderer()->OnGetFBO();
}

void* FlutterTizenView::OnProcResolver(const char* name) {
  return engine_->renderer()->OnProcResolver(name);
}

void FlutterTizenView::OnResize(int32_t left,
                                int32_t top,
                                int32_t width,
                                int32_t height) {
  if (rotation_degree_ == 90 || rotation_degree_ == 270) {
    std::swap(width, height);
  }

  engine_->renderer()->ResizeSurface(width, height);

  SendWindowMetrics(left, top, width, height, 0.0);
}

void FlutterTizenView::OnRotate(int32_t degree) {
  TizenGeometry geometry = tizen_view_->GetGeometry();
  int32_t width = geometry.width;
  int32_t height = geometry.height;
  if (dynamic_cast<TizenRendererEgl*>(engine_->renderer())) {
    rotation_degree_ = degree;
    // Compute renderer transformation based on the angle of rotation.
    double rad = (360 - rotation_degree_) * M_PI / 180;
    double trans_x = 0.0, trans_y = 0.0;
    if (rotation_degree_ == 90) {
      trans_y = height;
    } else if (rotation_degree_ == 180) {
      trans_x = width;
      trans_y = height;
    } else if (rotation_degree_ == 270) {
      trans_x = width;
    }

    flutter_transformation_ = {
        cos(rad), -sin(rad), trans_x,  // x
        sin(rad), cos(rad),  trans_y,  // y
        0.0,      0.0,       1.0       // perspective
    };

    if (rotation_degree_ == 90 || rotation_degree_ == 270) {
      std::swap(width, height);
    }
  }

  engine_->renderer()->ResizeSurface(width, height);

  // Window position does not change on rotation regardless of its orientation.
  SendWindowMetrics(geometry.left, geometry.top, width, height, 0.0);
}

FlutterTizenView::PointerState* FlutterTizenView::GetOrCreatePointerState(
    FlutterPointerDeviceKind device_kind,
    int32_t device_id) {
  // Create a virtual pointer ID that is unique across all device types
  // to prevent pointers from clashing in the engine's converter
  // (lib/ui/window/pointer_data_packet_converter.cc)
  int32_t pointer_id = (static_cast<int32_t>(device_kind) << 28) | device_id;

  auto [iter, added] = pointer_states_.try_emplace(pointer_id, nullptr);
  if (added) {
    auto state = std::make_unique<PointerState>();
    state->device_kind = device_kind;
    state->pointer_id = pointer_id;
    iter->second = std::move(state);
  }
  return iter->second.get();
}

FlutterPointerPhase FlutterTizenView::GetPointerPhaseFromState(
    const PointerState* state) const {
  // For details about this logic, see FlutterPointerPhase in the embedder.h
  // file.
  if (state->buttons == 0) {
    return state->flutter_state_is_down ? FlutterPointerPhase::kUp
                                        : FlutterPointerPhase::kHover;
  } else {
    return state->flutter_state_is_down ? FlutterPointerPhase::kMove
                                        : FlutterPointerPhase::kDown;
  }
}

void FlutterTizenView::OnPointerMove(double x,
                                     double y,
                                     size_t timestamp,
                                     FlutterPointerDeviceKind device_kind,
                                     int32_t device_id) {
  PointerState* state = GetOrCreatePointerState(device_kind, device_id);
  FlutterPointerPhase phase = GetPointerPhaseFromState(state);
  SendFlutterPointerEvent(phase, x, y, 0, 0, timestamp, state);
}

void FlutterTizenView::OnPointerDown(double x,
                                     double y,
                                     FlutterPointerMouseButtons button,
                                     size_t timestamp,
                                     FlutterPointerDeviceKind device_kind,
                                     int32_t device_id) {
  if (button != 0) {
    PointerState* state = GetOrCreatePointerState(device_kind, device_id);
    state->buttons |= button;
    FlutterPointerPhase phase = GetPointerPhaseFromState(state);
    SendFlutterPointerEvent(phase, x, y, 0, 0, timestamp, state);

    state->flutter_state_is_down = true;
  }
}

void FlutterTizenView::OnPointerUp(double x,
                                   double y,
                                   FlutterPointerMouseButtons button,
                                   size_t timestamp,
                                   FlutterPointerDeviceKind device_kind,
                                   int32_t device_id) {
  if (button != 0) {
    PointerState* state = GetOrCreatePointerState(device_kind, device_id);
    state->buttons &= ~button;
    FlutterPointerPhase phase = GetPointerPhaseFromState(state);
    SendFlutterPointerEvent(phase, x, y, 0, 0, timestamp, state);

    if (phase == FlutterPointerPhase::kUp) {
      state->flutter_state_is_down = false;
    }
  }
}

void FlutterTizenView::OnScroll(double x,
                                double y,
                                double delta_x,
                                double delta_y,
                                size_t timestamp,
                                FlutterPointerDeviceKind device_kind,
                                int32_t device_id) {
  PointerState* state = GetOrCreatePointerState(device_kind, device_id);
  FlutterPointerPhase phase = GetPointerPhaseFromState(state);
  SendFlutterPointerEvent(phase, x, y, delta_x, delta_y, timestamp, state);
}

void FlutterTizenView::OnKey(const char* key,
                             const char* string,
                             const char* compose,
                             uint32_t modifiers,
                             uint32_t scan_code,
                             const char* device_name,
                             bool is_down) {
  if (is_down) {
    FT_LOG(Info) << "Key symbol: " << key << ", code: 0x" << std::setw(8)
                 << std::setfill('0') << std::right << std::hex << scan_code;
  }

  // Do not handle the TV system menu key.
  if (strcmp(key, kSysMenuKey) == 0) {
    return;
  }

  if (input_device_channel_ && is_down) {
    input_device_channel_->SetLastKeyboardInfo(
        device_name ? std::string(device_name) : std::string());
  }

  if (text_input_channel_) {
    if (text_input_channel_->SendKey(key, string, compose, modifiers, scan_code,
                                     is_down)) {
      return;
    }
  }

  if (platform_view_channel_) {
    if (platform_view_channel_->SendKey(key, string, compose, modifiers,
                                        scan_code, is_down)) {
      return;
    }
  }

  if (engine_->keyboard_channel()) {
    engine_->keyboard_channel()->SendKey(
        key, string, compose, modifiers, scan_code, is_down,
        [engine = engine_.get(), symbol = std::string(key),
         is_down](bool handled) {
          if (handled) {
            return;
          }
          if (symbol == kBackKey && !is_down) {
            if (engine->navigation_channel()) {
              engine->navigation_channel()->PopRoute();
            }
          } else if (symbol == kExitKey && !is_down) {
            ui_app_exit();
          }
        });
  }
}

void FlutterTizenView::OnComposeBegin() {
  text_input_channel_->OnComposeBegin();
}

void FlutterTizenView::OnComposeChange(const std::string& str, int cursor_pos) {
  text_input_channel_->OnComposeChange(str, cursor_pos);
}

void FlutterTizenView::OnComposeEnd() {
  text_input_channel_->OnComposeEnd();
}

void FlutterTizenView::OnCommit(const std::string& str) {
  text_input_channel_->OnCommit(str);
}

void FlutterTizenView::SendInitialGeometry() {
  if (auto* window = dynamic_cast<TizenWindow*>(tizen_view_.get())) {
    OnRotate(window->GetRotation());
  } else {
    TizenGeometry geometry = tizen_view_->GetGeometry();
    SendWindowMetrics(geometry.left, geometry.top, geometry.width,
                      geometry.height, 0.0);
  }
}

void FlutterTizenView::SendWindowMetrics(int32_t left,
                                         int32_t top,
                                         int32_t width,
                                         int32_t height,
                                         double pixel_ratio) {
  if (pixel_ratio == 0.0) {
    pixel_ratio = ComputePixelRatio(tizen_view_.get());
  }

  engine_->SendWindowMetrics(left, top, width, height, pixel_ratio);
}

void FlutterTizenView::SendFlutterPointerEvent(FlutterPointerPhase phase,
                                               double x,
                                               double y,
                                               double delta_x,
                                               double delta_y,
                                               size_t timestamp,
                                               PointerState* state) {
  TizenGeometry geometry = tizen_view_->GetGeometry();
  double new_x = x, new_y = y;

  if (rotation_degree_ == 90) {
    new_x = geometry.height - y;
    new_y = x;
  } else if (rotation_degree_ == 180) {
    new_x = geometry.width - x;
    new_y = geometry.height - y;
  } else if (rotation_degree_ == 270) {
    new_x = y;
    new_y = geometry.width - x;
  }

  // If the pointer isn't already added, synthesize an add to satisfy Flutter's
  // expectations about events.
  if (!state->flutter_state_is_added) {
    FlutterPointerEvent event = {};
    event.struct_size = sizeof(event);
    event.phase = FlutterPointerPhase::kAdd;
    event.x = new_x;
    event.y = new_y;
    event.buttons = 0;
    event.timestamp = timestamp * 1000;
    event.device = state->pointer_id;
    event.device_kind = state->device_kind;
    event.buttons = state->buttons;
    event.view_id = view_id();
    engine_->SendPointerEvent(event);

    state->flutter_state_is_added = true;
  }

  FlutterPointerEvent event = {};
  event.struct_size = sizeof(event);
  event.phase = phase;
  event.x = new_x;
  event.y = new_y;
  if (delta_x != 0 || delta_y != 0) {
    event.signal_kind = kFlutterPointerSignalKindScroll;
  }
  event.scroll_delta_x = delta_x * kScrollOffsetMultiplier;
  event.scroll_delta_y = delta_y * kScrollOffsetMultiplier;
  event.timestamp = timestamp * 1000;
  event.device = state->pointer_id;
  event.device_kind = state->device_kind;
  event.buttons = state->buttons;
  event.view_id = view_id();
  engine_->SendPointerEvent(event);
}

}  // namespace flutter
