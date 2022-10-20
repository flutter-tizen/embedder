// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_ATK_UTIL_AURALINUX_H_
#define UI_ACCESSIBILITY_PLATFORM_ATK_UTIL_AURALINUX_H_

#include <atk/atk.h>

#include "ax/ax_export.h"
#include "ax_platform_node_auralinux.h"
#include "base/macros.h"

namespace ui {

// This singleton class initializes ATK (accessibility toolkit) and
// registers an implementation of the AtkUtil class, a global class that
// every accessible application needs to register once.
class AX_EXPORT AtkUtilAuraLinux {
 public:
  // Get the single instance of this class.
  static AtkUtilAuraLinux& GetInstance();

  ~AtkUtilAuraLinux() = default;

  void InitializeAsync();

  bool IsAtSpiReady();
  void SetAtSpiReady(bool ready);

  // Nodes with postponed events will get the function RunPostponedEvents()
  // called as soon as AT-SPI is detected to be ready
  void PostponeEventsFor(AXPlatformNodeAuraLinux* node);

  void CancelPostponedEventsFor(AXPlatformNodeAuraLinux* node);

 private:
  friend struct base::DefaultSingletonTraits<AtkUtilAuraLinux>;

  AtkUtilAuraLinux() = default;

  void PlatformInitializeAsync();

  bool at_spi_ready_ = false;

  BASE_DISALLOW_COPY_AND_ASSIGN(AtkUtilAuraLinux);
};

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_ATK_UTIL_AURALINUX_H_
