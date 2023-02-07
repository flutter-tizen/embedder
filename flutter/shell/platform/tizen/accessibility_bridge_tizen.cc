// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "accessibility_bridge_tizen.h"

#include "flutter/shell/platform/tizen/flutter_platform_node_delegate_tizen.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

AccessibilityBridgeTizen::AccessibilityBridgeTizen(FlutterTizenEngine* engine)
    : engine_(engine) {}

void AccessibilityBridgeTizen::OnAccessibilityEvent(
    ui::AXEventGenerator::TargetedEvent targeted_event) {
  std::shared_ptr<FlutterPlatformNodeDelegate> platform_node_delegate =
      GetFlutterPlatformNodeDelegateFromID(targeted_event.node->id()).lock();
  if (!platform_node_delegate) {
    FT_LOG(Error) << "Platform node delegate is deallocated";
    return;
  }
  auto tizen_platform_node_delegate =
      std::static_pointer_cast<FlutterPlatformNodeDelegateTizen>(
          platform_node_delegate);
  tizen_platform_node_delegate->NotifyAccessibilityEvent(
      targeted_event.event_params.event);
}

void AccessibilityBridgeTizen::DispatchAccessibilityAction(
    AccessibilityNodeId target,
    FlutterSemanticsAction action,
    fml::MallocMapping data) {
  engine_->DispatchAccessibilityAction(target, action, std::move(data));
}

std::shared_ptr<FlutterPlatformNodeDelegate>
AccessibilityBridgeTizen::CreateFlutterPlatformNodeDelegate() {
  return std::make_shared<FlutterPlatformNodeDelegateTizen>();
}

}  // namespace flutter
