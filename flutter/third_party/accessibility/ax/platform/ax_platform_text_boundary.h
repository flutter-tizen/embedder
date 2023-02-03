// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_TEXT_BOUNDARY_H_
#define UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_TEXT_BOUNDARY_H_

#include "ax/ax_enums.h"
#include "ax/ax_export.h"
#include "ax_build/build_config.h"

#ifdef OS_LINUX
#include <atk/atk.h>
#endif  // OS_LINUX

namespace ui {

#ifdef OS_LINUX
// Converts from an ATK text boundary to an ax::mojom::TextBoundary.
AX_EXPORT ax::mojom::TextBoundary FromAtkTextBoundary(AtkTextBoundary boundary);

#if ATK_CHECK_VERSION(2, 10, 0)
// Same as above, but for an older version of the API.
AX_EXPORT ax::mojom::TextBoundary FromAtkTextGranularity(
    AtkTextGranularity granularity);
#endif  // ATK_CHECK_VERSION(2, 10, 0)
#endif  // OS_LINUX

}  // namespace ui

#endif  // UI_ACCESSIBILITY_PLATFORM_AX_PLATFORM_TEXT_BOUNDARY_H_
