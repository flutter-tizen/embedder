// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_view.h"

#include <vector>

#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/tizen/testing/engine_modifier.h"
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
  TizenGeometry GetGeometry() override { return {0, 0, 100, 100}; }
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

}  // namespace
}  // namespace testing
}  // namespace flutter
