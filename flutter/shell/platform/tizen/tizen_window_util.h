// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_WINDOW_UTIL_H_
#define EMBEDDER_TIZEN_WINDOW_UTIL_H_

#ifdef TV_PROFILE

#include <cstdint>

struct wl_display;
struct wl_registry;
struct wl_seat;
struct wl_surface;

namespace flutter {

// Loads the TV-specific library libvd-win-util.so and exposes the subset of
// its cursor / mouse-pointer / toast entry points that the window backends
// need.
//
// dlopen is used because neither the library nor its headers
// (vd-win-util's cursor_module.h) are present in the rootstrap. The symbols
// are internal to the Samsung TV platform and may be renamed or removed in
// future releases, so every accessor tolerates a missing library or symbol by
// logging and returning false. Callers must keep working with the
// corresponding feature disabled.
//
// The library is opened for the lifetime of the instance. Create one where the
// original per-call dlopen used to be; a single instance must stay alive
// across an InitializeCursorModule / SetCursorConfig / FinalizeCursorModule
// sequence.
class TizenWindowUtil {
 public:
  // Values are defined by libvd-win-util and must not be renumbered.
  enum MouseSupport { kMouseSupportDisable = 0, kMouseSupportEnable = 1 };
  enum DeviceType { kDeviceTypeMouse = 3, kDeviceTypeTouch = 4 };

  // The config_type 1 refers to TIZEN_CURSOR_CONFIG_CURSOR_AVAILABLE defined
  // in the TV extension protocol tizen-extension-tv.xml.
  static constexpr uint32_t kCursorConfigCursorAvailable = 1;

  TizenWindowUtil();
  ~TizenWindowUtil();

  // Prevent copying.
  TizenWindowUtil(const TizenWindowUtil&) = delete;
  TizenWindowUtil& operator=(const TizenWindowUtil&) = delete;

  // Returns true if the library and all required symbols were resolved.
  bool IsValid() const { return handle_ != nullptr; }

  bool InitializeCursorModule(wl_display* display,
                              wl_registry* registry,
                              wl_seat* seat,
                              unsigned int cursor_global_id);
  bool SetCursorConfig(wl_surface* surface, uint32_t config_type, void* data);
  void FinalizeCursorModule();

  // |native_window| is the backend-specific window handle
  // (Ecore_Wl2_Window* or tizen_core_wl_window_h).
  bool SetMousePointerSupport(bool enable, void* native_window);
  bool SetMousePointerNotAllow(bool not_allow, void* native_window);
  bool LaunchUnsupportedToast(DeviceType type,
                              bool show,
                              bool enable,
                              void* native_window);

 private:
  // Resolves |name| from the library, logging on failure.
  void* Resolve(const char* name);

  void* handle_ = nullptr;

  int (*cursor_module_initialize_)(wl_display* display,
                                   wl_registry* registry,
                                   wl_seat* seat,
                                   unsigned int id) = nullptr;
  int (*cursor_set_config_)(wl_surface* surface,
                            uint32_t config_type,
                            void* data) = nullptr;
  void (*cursor_module_finalize_)(void) = nullptr;
  int (*mouse_pointer_support_)(int type, void* window) = nullptr;
  int (*mouse_pointer_not_allow_)(int enable, void* window) = nullptr;
  void (*unsupported_toast_launch_)(int type,
                                    int show,
                                    int enable,
                                    void* window) = nullptr;
};

}  // namespace flutter

#endif  // TV_PROFILE

#endif  // EMBEDDER_TIZEN_WINDOW_UTIL_H_
