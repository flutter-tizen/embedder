// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_platform_node_delegate_tizen.h"

#include <app_common.h>

#include "flutter/shell/platform/common/accessibility_bridge.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/third_party/accessibility/ax/platform/ax_platform_node_auralinux.h"

namespace flutter {

FlutterPlatformNodeDelegateTizen::FlutterPlatformNodeDelegateTizen() {}

FlutterPlatformNodeDelegateTizen::~FlutterPlatformNodeDelegateTizen() {
  if (platform_node_) {
    platform_node_->Destroy();
    platform_node_ = nullptr;
  }
}

void FlutterPlatformNodeDelegateTizen::Init(std::weak_ptr<OwnerBridge> bridge,
                                            ui::AXNode* node) {
  FlutterPlatformNodeDelegate::Init(bridge, node);
  platform_node_ = ui::AXPlatformNode::Create(this);
  FT_LOG(Debug) << "Create platform node for AXNode "
                << node->data().ToString();
}

void FlutterPlatformNodeDelegateTizen::NotifyAccessibilityEvent(
    ui::AXEventGenerator::Event event_type) {
  if (!platform_node_) {
    FT_LOG(Error) << "Platform node isn't created";
    return;
  }

  auto paltform_node =
      static_cast<ui::AXPlatformNodeAuraLinux*>(platform_node_);
  FT_LOG(Debug) << "Handle event " << ui::ToString(event_type) << " on node "
                << GetData().id;
  switch (event_type) {
    case ui::AXEventGenerator::Event::ACTIVE_DESCENDANT_CHANGED:
      paltform_node->NotifyAccessibilityEvent(
          ax::mojom::Event::kActiveDescendantChanged);
      break;
    case ui::AXEventGenerator::Event::ALERT:
      paltform_node->NotifyAccessibilityEvent(ax::mojom::Event::kAlert);
      break;
    case ui::AXEventGenerator::Event::CHECKED_STATE_CHANGED:
      paltform_node->NotifyAccessibilityEvent(
          ax::mojom::Event::kCheckedStateChanged);
      break;
    case ui::AXEventGenerator::Event::DESCRIPTION_CHANGED:
      paltform_node->OnDescriptionChanged();
      break;
    case ui::AXEventGenerator::Event::DOCUMENT_TITLE_CHANGED:
      paltform_node->NotifyAccessibilityEvent(
          ax::mojom::Event::kDocumentTitleChanged);
      break;
    case ui::AXEventGenerator::Event::ENABLED_CHANGED:
      paltform_node->OnEnabledChanged();
      break;
    case ui::AXEventGenerator::Event::EXPANDED:
      paltform_node->NotifyAccessibilityEvent(
          ax::mojom::Event::kExpandedChanged);
      break;
    case ui::AXEventGenerator::Event::FOCUS_CHANGED:
      paltform_node->NotifyAccessibilityEvent(ax::mojom::Event::kFocus);
      break;
    case ui::AXEventGenerator::Event::INVALID_STATUS_CHANGED:
      paltform_node->NotifyAccessibilityEvent(
          ax::mojom::Event::kInvalidStatusChanged);
      break;
    case ui::AXEventGenerator::Event::LOAD_COMPLETE:
      paltform_node->NotifyAccessibilityEvent(ax::mojom::Event::kLoadComplete);
      break;
    case ui::AXEventGenerator::Event::SELECTED_CHANGED:
      paltform_node->NotifyAccessibilityEvent(ax::mojom::Event::kSelection);
      break;
    case ui::AXEventGenerator::Event::SUBTREE_CREATED:
      paltform_node->OnSubtreeCreated();
      break;
    case ui::AXEventGenerator::Event::TEXT_ATTRIBUTE_CHANGED:
      paltform_node->OnTextAttributesChanged();
      break;
    case ui::AXEventGenerator::Event::VALUE_CHANGED:
      paltform_node->NotifyAccessibilityEvent(ax::mojom::Event::kValueChanged);
      break;
    default:
      break;
  }
}

gfx::NativeViewAccessible
FlutterPlatformNodeDelegateTizen::GetNativeViewAccessible() {
  if (!platform_node_) {
    FT_LOG(Error) << "Platform node isn't created";
    return nullptr;
  }
  return platform_node_->GetNativeViewAccessible();
}

gfx::NativeViewAccessible FlutterPlatformNodeDelegateTizen::GetParent() {
  gfx::NativeViewAccessible parent = FlutterPlatformNodeDelegate::GetParent();
  if (!parent) {
    parent = FlutterPlatformAppDelegateTizen::GetInstance()
                 .GetWindow()
                 .lock()
                 ->GetNativeViewAccessible();
  }
  return parent;
}

gfx::Rect FlutterPlatformNodeDelegateTizen::GetBoundsRect(
    const ui::AXCoordinateSystem coordinate_system,
    const ui::AXClippingBehavior clipping_behavior,
    ui::AXOffscreenResult* offscreen_result) const {
  gfx::Rect bounds = FlutterPlatformNodeDelegate::GetBoundsRect(
      coordinate_system, clipping_behavior, offscreen_result);
  FT_LOG(Debug) << "Node " << GetAXNode()->id() << " bounds: x=" << bounds.x()
                << ", y=" << bounds.y() << ", width=" << bounds.width()
                << ", height=" << bounds.height();
  return bounds;
}

bool FlutterPlatformNodeDelegateTizen::HitTestPoint(
    const gfx::Point& point) const {
  gfx::Rect bounds =
      GetBoundsRect(ui::AXCoordinateSystem::kScreenPhysicalPixels,
                    ui::AXClippingBehavior::kUnclipped, nullptr);
  return bounds.Contains(point);
}

gfx::NativeViewAccessible FlutterPlatformNodeDelegateTizen::HitTestSync(
    int screen_physical_pixel_x,
    int screen_physical_pixel_y) const {
  gfx::NativeViewAccessible result = nullptr;
  gfx::Point point(screen_physical_pixel_x, screen_physical_pixel_y);
  ui::AXNode* ax_node = GetAXNode();
  auto bridge =
      std::static_pointer_cast<AccessibilityBridge>(GetOwnerBridge().lock());
  for (auto iter = ax_node->UnignoredChildrenBegin();
       iter != ax_node->UnignoredChildrenEnd(); ++iter) {
    auto child = std::static_pointer_cast<FlutterPlatformNodeDelegateTizen>(
        bridge->GetFlutterPlatformNodeDelegateFromID(iter.get()->id()).lock());
    if (child->HitTestPoint(point)) {
      result = child->GetNativeViewAccessible();
      break;
    }
  }

  if (!result) {
    if (HitTestPoint(point) && platform_node_) {
      result = platform_node_->GetNativeViewAccessible();
    }
  }

  return result;
}

FlutterPlatformWindowDelegateTizen::FlutterPlatformWindowDelegateTizen() {
  activated_ = false;
  data_.role = ax::mojom::Role::kWindow;
  platform_node_ = ui::AXPlatformNode::Create(this);
}

FlutterPlatformWindowDelegateTizen::~FlutterPlatformWindowDelegateTizen() {
  if (activated_) {
    static_cast<ui::AXPlatformNodeAuraLinux*>(platform_node_)
        ->NotifyAccessibilityEvent(ax::mojom::Event::kWindowDeactivated);
  }
  platform_node_->Destroy();
  platform_node_ = nullptr;
}

void FlutterPlatformWindowDelegateTizen::SetGeometry(int32_t x,
                                                     int32_t y,
                                                     int32_t width,
                                                     int32_t height) {
  geometry_ = gfx::Rect(x, y, width, height);
}

void FlutterPlatformWindowDelegateTizen::SetRootNode(
    std::weak_ptr<FlutterPlatformNodeDelegate> node) {
  root_ = node;
  static_cast<ui::AXPlatformNodeAuraLinux*>(platform_node_)
      ->NotifyAccessibilityEvent(ax::mojom::Event::kWindowActivated);
  activated_ = true;
}

const ui::AXNodeData& FlutterPlatformWindowDelegateTizen::GetData() const {
  return data_;
}

gfx::NativeViewAccessible
FlutterPlatformWindowDelegateTizen::GetNativeViewAccessible() {
  return platform_node_->GetNativeViewAccessible();
}

gfx::NativeViewAccessible FlutterPlatformWindowDelegateTizen::GetParent() {
  return FlutterPlatformAppDelegateTizen::GetInstance()
      .GetNativeViewAccessible();
}

int FlutterPlatformWindowDelegateTizen::GetChildCount() const {
  if (root_.expired()) {
    return 0;
  } else {
    return 1;
  }
}

gfx::NativeViewAccessible FlutterPlatformWindowDelegateTizen::ChildAtIndex(
    int index) {
  if (index < 0 || index >= GetChildCount()) {
    return nullptr;
  }

  return root_.lock()->GetNativeViewAccessible();
}

gfx::Rect FlutterPlatformWindowDelegateTizen::GetBoundsRect(
    const ui::AXCoordinateSystem coordinate_system,
    const ui::AXClippingBehavior clipping_behavior,
    ui::AXOffscreenResult* offscreen_result) const {
  // TODO: assuming the screen dpr is 1.0
  return geometry_;
}

gfx::NativeViewAccessible FlutterPlatformWindowDelegateTizen::HitTestSync(
    int screen_physical_pixel_x,
    int screen_physical_pixel_y) const {
  gfx::Point point(screen_physical_pixel_x, screen_physical_pixel_y);
  if (!root_.expired()) {
    auto node = std::static_pointer_cast<FlutterPlatformNodeDelegateTizen>(
        root_.lock());
    if (node->HitTestPoint(point)) {
      return node->HitTestSync(screen_physical_pixel_x,
                               screen_physical_pixel_y);
    }
  }

  gfx::Rect bounds =
      GetBoundsRect(ui::AXCoordinateSystem::kScreenPhysicalPixels,
                    ui::AXClippingBehavior::kUnclipped, nullptr);
  if (bounds.Contains(point)) {
    return platform_node_->GetNativeViewAccessible();
  } else {
    return nullptr;
  }
}

FlutterPlatformAppDelegateTizen::FlutterPlatformAppDelegateTizen() {
  ui::AXPlatformNodeAuraLinux::EnableAXMode();
  data_.role = ax::mojom::Role::kApplication;
  platform_node_ = ui::AXPlatformNode::Create(this);

  char* name = nullptr;
  int result = app_get_name(&name);
  if (result != APP_ERROR_NONE || name == nullptr) {
    FT_LOG(Error) << "Can't get app name.";
  } else {
    FT_LOG(Debug) << "App name: " << name;
    data_.AddStringAttribute(ax::mojom::StringAttribute::kName, name);
    free(name);
  }

  ui::AXPlatformNodeAuraLinux::SetApplication(platform_node_);
  ui::AXPlatformNodeAuraLinux::StaticInitialize();
}

FlutterPlatformAppDelegateTizen::~FlutterPlatformAppDelegateTizen() {
  platform_node_->Destroy();
  platform_node_ = nullptr;
}

// static
FlutterPlatformAppDelegateTizen&
FlutterPlatformAppDelegateTizen::GetInstance() {
  static FlutterPlatformAppDelegateTizen instance;
  return instance;
}

std::weak_ptr<FlutterPlatformWindowDelegateTizen>
FlutterPlatformAppDelegateTizen::GetWindow() {
  return window_;
}

void FlutterPlatformAppDelegateTizen::SetAccessibilityStatus(bool enabled) {
  if (!enabled && window_) {
    window_.reset();
  } else if (enabled && !window_) {
    window_ = std::make_shared<FlutterPlatformWindowDelegateTizen>();
  }
}

const ui::AXNodeData& FlutterPlatformAppDelegateTizen::GetData() const {
  return data_;
}

gfx::NativeViewAccessible
FlutterPlatformAppDelegateTizen::GetNativeViewAccessible() {
  if (platform_node_) {
    return platform_node_->GetNativeViewAccessible();
  } else {
    return nullptr;
  }
}

int FlutterPlatformAppDelegateTizen::GetChildCount() const {
  if (window_) {
    return 1;
  } else {
    return 0;
  }
}

gfx::NativeViewAccessible FlutterPlatformAppDelegateTizen::ChildAtIndex(
    int index) {
  if (index < 0 || index >= GetChildCount()) {
    return nullptr;
  }

  return window_->GetNativeViewAccessible();
}

}  // namespace flutter
