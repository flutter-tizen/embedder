// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tizen_shell.h"

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

TizenShell::TizenShell() {
  tizen_shell_ = tzsh_create(TZSH_TOOLKIT_TYPE_EFL);
  if (!tizen_shell_) {
    FT_LOG(Error) << "tzsh_create() failed with error: "
                  << get_error_message(get_last_result());
  }
}

TizenShell::~TizenShell() {
  if (tizen_shell_softkey_) {
    tzsh_softkey_destroy(tizen_shell_softkey_);
  }
  if (tizen_shell_) {
    tzsh_destroy(tizen_shell_);
  }
}

void TizenShell::InitializeSoftkey(uint32_t window_id) {
  if (tizen_shell_softkey_ || !tizen_shell_) {
    return;
  }
  tizen_shell_softkey_ = tzsh_softkey_create(tizen_shell_, window_id);
  if (!tizen_shell_softkey_) {
    int ret = get_last_result();
    if (ret == TZSH_ERROR_PERMISSION_DENIED) {
      FT_LOG(Error) << "Permission denied. You need a "
                       "\"http://tizen.org/privilege/windowsystem.admin\" "
                       "privilege to use this method.";
    } else {
      FT_LOG(Error) << "tzsh_softkey_create() failed with error: "
                    << get_error_message(ret);
    }
  }
}

void TizenShell::ShowSoftkey() {
  if (!tizen_shell_softkey_) {
    return;
  }
  int ret = tzsh_softkey_global_show(tizen_shell_softkey_);
  if (ret != TZSH_ERROR_NONE) {
    FT_LOG(Error) << "tzsh_softkey_global_show() failed with error: "
                  << get_error_message(ret);
    return;
  }
  is_softkey_shown_ = true;
}

void TizenShell::HideSoftkey() {
  if (!tizen_shell_softkey_) {
    return;
  }
  // Always show the softkey before hiding it again, to avoid subtle bugs.
  tzsh_softkey_global_show(tizen_shell_softkey_);
  int ret = tzsh_softkey_global_hide(tizen_shell_softkey_);
  if (ret != TZSH_ERROR_NONE) {
    FT_LOG(Error) << "tzsh_softkey_global_hide() failed with error: "
                  << get_error_message(ret);
    return;
  }
  is_softkey_shown_ = false;
}

}  // namespace flutter
