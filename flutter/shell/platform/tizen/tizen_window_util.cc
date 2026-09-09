// Copyright 2026 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_window_util.h"

#ifdef TV_PROFILE

#include <dlfcn.h>

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kLibraryName[] = "libvd-win-util.so";

}  // namespace

TizenWindowUtil::TizenWindowUtil() {
  handle_ = dlopen(kLibraryName, RTLD_LAZY);
  if (!handle_) {
    FT_LOG(Error) << "Could not open a shared library " << kLibraryName << ".";
    return;
  }

  *(void**)(&cursor_module_initialize_) = Resolve("CursorModule_Initialize");
  *(void**)(&cursor_set_config_) = Resolve("Cursor_Set_Config");
  *(void**)(&cursor_module_finalize_) = Resolve("CursorModule_Finalize");
  *(void**)(&mouse_pointer_support_) = Resolve("Mouse_Pointer_Support");
  *(void**)(&mouse_pointer_not_allow_) = Resolve("Mouse_Pointer_Not_Allow");
  *(void**)(&unsupported_toast_launch_) = Resolve("Unsupported_Toast_Launch");
}

TizenWindowUtil::~TizenWindowUtil() {
  if (handle_) {
    dlclose(handle_);
    handle_ = nullptr;
  }
}

void* TizenWindowUtil::Resolve(const char* name) {
  void* symbol = dlsym(handle_, name);
  if (!symbol) {
    FT_LOG(Error) << "Could not load the symbol " << name << " from "
                  << kLibraryName << ".";
  }
  return symbol;
}

bool TizenWindowUtil::InitializeCursorModule(wl_display* display,
                                             wl_registry* registry,
                                             wl_seat* seat,
                                             unsigned int cursor_global_id) {
  if (!cursor_module_initialize_) {
    return false;
  }
  if (!cursor_module_initialize_(display, registry, seat, cursor_global_id)) {
    FT_LOG(Error) << "Failed to initialize the cursor module.";
    return false;
  }
  return true;
}

bool TizenWindowUtil::SetCursorConfig(wl_surface* surface,
                                      uint32_t config_type,
                                      void* data) {
  if (!cursor_set_config_) {
    return false;
  }
  if (!cursor_set_config_(surface, config_type, data)) {
    FT_LOG(Error) << "Failed to set a cursor config value.";
    return false;
  }
  return true;
}

void TizenWindowUtil::FinalizeCursorModule() {
  if (cursor_module_finalize_) {
    cursor_module_finalize_();
  }
}

bool TizenWindowUtil::SetMousePointerSupport(bool enable, void* native_window) {
  if (!mouse_pointer_support_) {
    return false;
  }
  mouse_pointer_support_(enable ? kMouseSupportEnable : kMouseSupportDisable,
                         native_window);
  return true;
}

bool TizenWindowUtil::SetMousePointerNotAllow(bool not_allow,
                                              void* native_window) {
  if (!mouse_pointer_not_allow_) {
    return false;
  }
  mouse_pointer_not_allow_(not_allow, native_window);
  return true;
}

bool TizenWindowUtil::LaunchUnsupportedToast(DeviceType type,
                                             bool show,
                                             bool enable,
                                             void* native_window) {
  if (!unsupported_toast_launch_) {
    return false;
  }
  unsupported_toast_launch_(type, show, enable, native_window);
  return true;
}

}  // namespace flutter

#endif  // TV_PROFILE
