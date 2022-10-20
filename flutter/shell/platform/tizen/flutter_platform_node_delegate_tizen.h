// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_PLATFORM_NODE_DELEGATE_TIZEN_H_
#define EMBEDDER_FLUTTER_PLATFORM_NODE_DELEGATE_TIZEN_H_

#include "flutter/shell/platform/common/flutter_platform_node_delegate.h"

namespace flutter {

// The Tizen implementation of FlutterPlatformNodeDelegate.
class FlutterPlatformNodeDelegateTizen : public FlutterPlatformNodeDelegate {
 public:
  FlutterPlatformNodeDelegateTizen();
  virtual ~FlutterPlatformNodeDelegateTizen();

  void NotifyAccessibilityEvent(ui::AXEventGenerator::Event event_type);

  // |ui::AXPlatformNodeDelegate|
  gfx::NativeViewAccessible GetNativeViewAccessible() override;

  // |FlutterPlatformNodeDelegate|
  gfx::NativeViewAccessible GetParent() override;

  // |FlutterPlatformNodeDelegate|
  gfx::Rect GetBoundsRect(
      const ui::AXCoordinateSystem coordinate_system,
      const ui::AXClippingBehavior clipping_behavior,
      ui::AXOffscreenResult* offscreen_result) const override;

  bool HitTestPoint(const gfx::Point& point) const;

  // |ui::AXPlatformNodeDelegate|
  gfx::NativeViewAccessible HitTestSync(
      int screen_physical_pixel_x,
      int screen_physical_pixel_y) const override;

  // |FlutterPlatformNodeDelegate|
  void Init(std::weak_ptr<OwnerBridge> bridge, ui::AXNode* node) override;

 private:
  ui::AXPlatformNode* platform_node_;
};

class FlutterPlatformWindowDelegateTizen
    : public ui::AXPlatformNodeDelegateBase {
 public:
  FlutterPlatformWindowDelegateTizen();
  virtual ~FlutterPlatformWindowDelegateTizen();

  void SetGeometry(int32_t x, int32_t y, int32_t width, int32_t height);
  void SetRootNode(std::weak_ptr<FlutterPlatformNodeDelegate> node);

  // |ui::AXPlatformNodeDelegateBase|
  const ui::AXNodeData& GetData() const override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::NativeViewAccessible GetNativeViewAccessible() override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::NativeViewAccessible GetParent() override;

  // |ui::AXPlatformNodeDelegateBase|
  int GetChildCount() const override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::NativeViewAccessible ChildAtIndex(int index) override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::Rect GetBoundsRect(
      const ui::AXCoordinateSystem coordinate_system,
      const ui::AXClippingBehavior clipping_behavior,
      ui::AXOffscreenResult* offscreen_result) const override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::NativeViewAccessible HitTestSync(
      int screen_physical_pixel_x,
      int screen_physical_pixel_y) const override;

 private:
  gfx::Rect geometry_;
  bool activated_;
  std::weak_ptr<FlutterPlatformNodeDelegate> root_;
  ui::AXPlatformNode* platform_node_;
  ui::AXNodeData data_;
};

class FlutterPlatformAppDelegateTizen : public ui::AXPlatformNodeDelegateBase {
 public:
  virtual ~FlutterPlatformAppDelegateTizen();

  FlutterPlatformAppDelegateTizen(const FlutterPlatformAppDelegateTizen&) =
      delete;
  FlutterPlatformAppDelegateTizen& operator=(
      const FlutterPlatformAppDelegateTizen&) = delete;

  static FlutterPlatformAppDelegateTizen& GetInstance();

  std::weak_ptr<FlutterPlatformWindowDelegateTizen> GetWindow();
  void SetAccessibilityStatus(bool enabled);

  // |ui::AXPlatformNodeDelegateBase|
  const ui::AXNodeData& GetData() const override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::NativeViewAccessible GetNativeViewAccessible() override;

  // |ui::AXPlatformNodeDelegateBase|
  int GetChildCount() const override;

  // |ui::AXPlatformNodeDelegateBase|
  gfx::NativeViewAccessible ChildAtIndex(int index) override;

 private:
  FlutterPlatformAppDelegateTizen();

  std::shared_ptr<FlutterPlatformWindowDelegateTizen> window_;
  ui::AXPlatformNode* platform_node_;
  ui::AXNodeData data_;
};

}  // namespace flutter

#endif  // EMBEDDER_FLUTTER_PLATFORM_NODE_DELEGATE_TIZEN_H_
