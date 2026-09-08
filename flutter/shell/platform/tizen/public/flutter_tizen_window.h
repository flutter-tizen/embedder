// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_WINDOW_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_WINDOW_H_

#include <stddef.h>
#include <stdint.h>

#include "flutter_export.h"
#include "flutter_tizen.h"

#if defined(__cplusplus)
extern "C" {
#endif

// The geometry (position and size) of a window.
typedef struct {
  // The x-coordinate of the top left corner of the window.
  int32_t x;
  // The y-coordinate of the top left corner of the window.
  int32_t y;
  // The width of the window.
  int32_t width;
  // The height of the window.
  int32_t height;
} FlutterDesktopWindowGeometry;

// The geometry (size) of the display screen.
typedef struct {
  // The width of the screen.
  int32_t width;
  // The height of the screen.
  int32_t height;
} FlutterDesktopScreenGeometry;

// ========== Window ==========

// Returns the geometry (position and size) of the window associated with
// the given view.
FLUTTER_EXPORT FlutterDesktopWindowGeometry
FlutterDesktopWindowGetGeometry(FlutterDesktopViewRef view);

// Sets the geometry (position and size) of the window associated with
// the given view.
//
// If any field of |geometry| is -1, the current value for that field is
// preserved.
//
// Returns true if the geometry was successfully set, false otherwise.
FLUTTER_EXPORT bool FlutterDesktopWindowSetGeometry(
    FlutterDesktopViewRef view,
    const FlutterDesktopWindowGeometry* geometry);

// Returns the geometry (size) of the screen that the window is on.
FLUTTER_EXPORT FlutterDesktopScreenGeometry
FlutterDesktopWindowGetScreenGeometry(FlutterDesktopViewRef view);

// Returns the current rotation (in degrees) of the window.
FLUTTER_EXPORT int32_t
FlutterDesktopWindowGetRotation(FlutterDesktopViewRef view);

// Activates the window associated with the given view.
FLUTTER_EXPORT void FlutterDesktopWindowActivate(FlutterDesktopViewRef view);

// Raises the window associated with the given view to the top of the
// stacking order.
FLUTTER_EXPORT void FlutterDesktopWindowRaise(FlutterDesktopViewRef view);

// Lowers the window associated with the given view to the bottom of the
// stacking order.
FLUTTER_EXPORT void FlutterDesktopWindowLower(FlutterDesktopViewRef view);

#if defined(__cplusplus)
}  // extern "C"
#endif

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_PUBLIC_FLUTTER_TIZEN_WINDOW_H_
