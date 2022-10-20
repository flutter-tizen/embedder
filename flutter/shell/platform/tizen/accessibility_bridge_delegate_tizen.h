// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ACCESSIBILITY_BRIDGE_DELEGATE_TIZEN_H_
#define EMBEDDER_ACCESSIBILITY_BRIDGE_DELEGATE_TIZEN_H_

#include "flutter/shell/platform/common/accessibility_bridge.h"

namespace flutter {

class FlutterTizenEngine;

// The Tizen implementation of AccessibilityBridge::AccessibilityBridgeDelegate.
// This delegate is used to create AccessibilityBridge in the Tizen platform.
class AccessibilityBridgeDelegateTizen
    : public AccessibilityBridge::AccessibilityBridgeDelegate {
 public:
  explicit AccessibilityBridgeDelegateTizen(FlutterTizenEngine* engine);
  virtual ~AccessibilityBridgeDelegateTizen() = default;

  // |AccessibilityBridge::AccessibilityBridgeDelegate|
  void OnAccessibilityEvent(
      ui::AXEventGenerator::TargetedEvent targeted_event) override;

  // |AccessibilityBridge::AccessibilityBridgeDelegate|
  void DispatchAccessibilityAction(AccessibilityNodeId target,
                                   FlutterSemanticsAction action,
                                   fml::MallocMapping data) override;

  // |AccessibilityBridge::AccessibilityBridgeDelegate|
  std::shared_ptr<FlutterPlatformNodeDelegate>
  CreateFlutterPlatformNodeDelegate() override;

 private:
  FlutterTizenEngine* engine_;
};

}  // namespace flutter

#endif  // EMBEDDER_ACCESSIBILITY_BRIDGE_DELEGATE_TIZEN_H_
