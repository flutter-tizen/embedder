// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_ACCESSIBILITY_BRIDGE_TIZEN_H_
#define EMBEDDER_ACCESSIBILITY_BRIDGE_TIZEN_H_

#include "flutter/shell/platform/common/accessibility_bridge.h"

namespace flutter {

class FlutterTizenEngine;

// The Tizen implementation of AccessibilityBridge.
class AccessibilityBridgeTizen : public AccessibilityBridge {
 public:
  AccessibilityBridgeTizen(FlutterTizenEngine* engine);
  virtual ~AccessibilityBridgeTizen() = default;

  // |AccessibilityBridge|
  void DispatchAccessibilityAction(AccessibilityNodeId target,
                                   FlutterSemanticsAction action,
                                   fml::MallocMapping data) override;

 protected:
  // |AccessibilityBridge|
  void OnAccessibilityEvent(
      ui::AXEventGenerator::TargetedEvent targeted_event) override;

  // |AccessibilityBridge|
  std::shared_ptr<FlutterPlatformNodeDelegate>
  CreateFlutterPlatformNodeDelegate() override;

 private:
  FlutterTizenEngine* engine_;
};

}  // namespace flutter

#endif  // EMBEDDER_ACCESSIBILITY_BRIDGE_TIZEN_H_
