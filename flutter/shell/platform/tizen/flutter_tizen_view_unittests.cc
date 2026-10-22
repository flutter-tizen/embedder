// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_view.h"

#include <vector>

#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/tizen/testing/engine_modifier.h"
#include "flutter/shell/platform/tizen/tizen_renderer_egl.h"
#include "flutter/shell/platform/tizen/tizen_window.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {
namespace {

class TestTizenView : public TizenWindow {
 public:
  TestTizenView() : TizenWindow({}, false, true, false) {
    input_method_context_ = std::make_unique<TizenInputMethodContext>(0);
  }

  void* GetRenderTarget() override { return nullptr; }
  void* GetNativeHandle() override { return nullptr; }
  uintptr_t GetWindowId() override { return 0; }
  TizenGeometry GetGeometry() override { return geometry; }
  bool SetGeometry(TizenGeometry geometry) override { return true; }
  int32_t GetDpi() override { return 160; }
  uint32_t GetResourceId() override { return 0; }
  void UpdateFlutterCursor(const std::string& kind) override {}
  void Show() override {}
  int32_t GetRotation() override { return 0; }
  void SetPreferredOrientations(const std::vector<int>& rotations) override {}
  void* GetRenderTargetDisplay() override { return nullptr; }
  TizenGeometry GetScreenGeometry() override { return GetGeometry(); }
  void BindKeys(const std::vector<std::string>& keys) override {}
  void ActivateWindow() override { activated = true; }
  void RaiseWindow() override {}
  void LowerWindow() override {}

  void SetFocusable(bool focusable) { focusable_ = focusable; }

  TizenGeometry geometry = {0, 0, 100, 100};
  bool activated = false;
};

TEST(FlutterTizenViewTest, SendsAndRequestsViewFocus) {
  {
    FlutterDesktopEngineProperties properties = {};
    properties.assets_path = "/foo/flutter_assets";
    properties.icu_data_path = "/foo/icudtl.dat";
    properties.aot_library_path = "/foo/libapp.so";

    FlutterProjectBundle project(properties);
    auto engine = std::make_unique<FlutterTizenEngine>(project);
    std::vector<FlutterViewFocusEvent> sent_events;
    EngineModifier modifier(engine.get());
    modifier.SetEngine(reinterpret_cast<FLUTTER_API_SYMBOL(FlutterEngine)>(1));
    modifier.embedder_api().SendViewFocusEvent = MOCK_ENGINE_PROC(
        SendViewFocusEvent,
        ([&sent_events](auto engine, const FlutterViewFocusEvent* event) {
          sent_events.push_back(*event);
          return kSuccess;
        }));
    modifier.embedder_api().Shutdown = [](auto engine) { return kSuccess; };

    auto tizen_view = std::make_unique<TestTizenView>();
    TestTizenView* tizen_view_ptr = tizen_view.get();
    FlutterTizenView view(kImplicitViewId, std::move(tizen_view),
                          std::move(engine), kEVulkan);

    view.OnFocus(kFocused);
    view.OnFocus(kFocused);
    ASSERT_EQ(sent_events.size(), 1u);
    EXPECT_EQ(sent_events[0].struct_size, sizeof(FlutterViewFocusEvent));
    EXPECT_EQ(sent_events[0].view_id, kImplicitViewId);
    EXPECT_EQ(sent_events[0].state, kFocused);
    EXPECT_EQ(sent_events[0].direction, kUndefined);

    view.OnFocus(kUnfocused);
    view.OnFocus(kUnfocused);
    ASSERT_EQ(sent_events.size(), 2u);
    EXPECT_EQ(sent_events[1].state, kUnfocused);

    FlutterViewFocusChangeRequest request = {
        .struct_size = sizeof(FlutterViewFocusChangeRequest),
        .view_id = kImplicitViewId,
        .state = kUnfocused,
        .direction = kUndefined,
    };
    view.OnFocusChangeRequest(request);
    EXPECT_FALSE(tizen_view_ptr->activated);

    request.state = kFocused;
    request.view_id = kImplicitViewId + 1;
    view.OnFocusChangeRequest(request);
    EXPECT_FALSE(tizen_view_ptr->activated);

    request.view_id = kImplicitViewId;
    view.OnFocusChangeRequest(request);
    EXPECT_TRUE(tizen_view_ptr->activated);

    tizen_view_ptr->activated = false;
    tizen_view_ptr->SetFocusable(false);
    view.OnFocus(kFocused);
    EXPECT_EQ(sent_events.size(), 2u);
    view.OnFocusChangeRequest(request);
    EXPECT_FALSE(tizen_view_ptr->activated);
  }
}

class InputPanelInsetsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    FlutterDesktopEngineProperties properties = {};
    properties.assets_path = "/foo/flutter_assets";
    properties.icu_data_path = "/foo/icudtl.dat";
    properties.aot_library_path = "/foo/libapp.so";
    FlutterProjectBundle project(properties);
    auto engine = std::make_unique<FlutterTizenEngine>(project);
    modifier_ = std::make_unique<EngineModifier>(engine.get());
    modifier_->SetEngine(
        reinterpret_cast<FLUTTER_API_SYMBOL(FlutterEngine)>(1));
    modifier_->embedder_api().SendWindowMetricsEvent = MOCK_ENGINE_PROC(
        SendWindowMetricsEvent,
        ([this](auto engine, const FlutterWindowMetricsEvent* event) {
          events_.push_back(*event);
          return kSuccess;
        }));
    modifier_->embedder_api().Shutdown = [](auto engine) { return kSuccess; };
    auto native_view = std::make_unique<TestTizenView>();
    native_ = native_view.get();
    native_->geometry = {10, 20, 200, 100};
    // kEVulkan creates no renderer in this build. Install an EGL renderer
    // without a surface so that OnRotate and OnResize take the device path.
    view_ = std::make_unique<FlutterTizenView>(
        kImplicitViewId, std::move(native_view), std::move(engine), kEVulkan,
        2.0);
    modifier_->embedder_api().PostRenderThreadTask =
        [](auto engine, auto callback, void* data) {
          callback(data);
          return kSuccess;
        };
    modifier_->SetRenderer(std::make_unique<TizenRendererEgl>(nullptr, false));
  }

  const FlutterWindowMetricsEvent& last_event() { return events_.back(); }

  std::vector<FlutterWindowMetricsEvent> events_;
  std::unique_ptr<EngineModifier> modifier_;
  TestTizenView* native_ = nullptr;
  std::unique_ptr<FlutterTizenView> view_;
};

TEST_F(InputPanelInsetsTest, ReportsBottomInset) {
  view_->OnInputPanelGeometryChanged({0, 90, 300, 100});
  ASSERT_EQ(events_.size(), 1u);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_top, 0);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_right, 0);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 30);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_left, 0);
  EXPECT_DOUBLE_EQ(last_event().pixel_ratio, 2);
  EXPECT_EQ(last_event().left, 10u);
  EXPECT_EQ(last_event().top, 20u);
  EXPECT_EQ(last_event().width, 200u);
  EXPECT_EQ(last_event().height, 100u);
  EXPECT_EQ(last_event().view_id, kImplicitViewId);
}

TEST_F(InputPanelInsetsTest, ReportsEachEdge) {
  view_->OnInputPanelGeometryChanged({0, 0, 300, 300});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 100);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_top, 0);
  view_->OnInputPanelGeometryChanged({0, 0, 300, 40});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_top, 20);
  view_->OnInputPanelGeometryChanged({180, 0, 200, 200});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_right, 30);
  view_->OnInputPanelGeometryChanged({0, 0, 40, 200});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_left, 30);
}

TEST_F(InputPanelInsetsTest, IgnoresPanelsNotCoveringAnEdge) {
  for (const auto& panel : std::vector<InputPanelGeometry>{
           {},
           {0, 200, 300, 100},   // Below the window.
           {300, 80, 200, 100},  // Right of the window.
           {30, 40, 100, 30},    // Floating inside the window.
           {30, 80, 100, 40},    // Partial width.
           {0, 80, -1, 40},      // Invalid size.
       }) {
    view_->OnInputPanelGeometryChanged(panel);
    EXPECT_DOUBLE_EQ(last_event().physical_view_inset_top, 0);
    EXPECT_DOUBLE_EQ(last_event().physical_view_inset_right, 0);
    EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 0);
    EXPECT_DOUBLE_EQ(last_event().physical_view_inset_left, 0);
  }
}

TEST_F(InputPanelInsetsTest, ClearsInsetsOnHide) {
  view_->OnInputPanelGeometryChanged({0, 90, 300, 100});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 30);
  view_->OnInputPanelGeometryChanged({});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 0);
}

TEST_F(InputPanelInsetsTest, RecomputesInsetsOnResize) {
  view_->OnInputPanelGeometryChanged({0, 80, 300, 100});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 40);

  // The window manager resized the window to fit above the panel.
  native_->geometry.height = 60;
  view_->OnResize(10, 20, 200, 60);
  EXPECT_EQ(last_event().height, 60u);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 0);

  native_->geometry.height = 100;
  view_->OnResize(10, 20, 200, 100);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 40);
}

TEST_F(InputPanelInsetsTest, RotatesInsetsWithView) {
  view_->OnInputPanelGeometryChanged({0, 90, 300, 100});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 30);

  // The panel geometry is reported in native display coordinates, so a panel
  // at the bottom of the display moves to the left of the rotated view.
  view_->OnRotate(90);
  EXPECT_EQ(last_event().width, 100u);
  EXPECT_EQ(last_event().height, 200u);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_left, 30);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 0);
  view_->OnRotate(180);
  EXPECT_EQ(last_event().width, 200u);
  EXPECT_EQ(last_event().height, 100u);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_top, 30);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_left, 0);
  view_->OnRotate(270);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_right, 30);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_top, 0);
  view_->OnRotate(0);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 30);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_right, 0);

  // A panel on the right of the display is at the bottom of a 90-degree view.
  view_->OnRotate(90);
  view_->OnInputPanelGeometryChanged({150, 0, 100, 200});
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_bottom, 60);
  EXPECT_DOUBLE_EQ(last_event().physical_view_inset_left, 0);
}

TEST_F(InputPanelInsetsTest, SkipsMetricsWhenEngineIsNotRunning) {
  modifier_->SetEngine(nullptr);
  view_->OnInputPanelGeometryChanged({0, 90, 300, 100});
  EXPECT_TRUE(events_.empty());
}

}  // namespace
}  // namespace testing
}  // namespace flutter
