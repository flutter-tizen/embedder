// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_SHELL_H_
#define EMBEDDER_TIZEN_SHELL_H_

#include <tzsh.h>
#include <tzsh_softkey.h>

#include <cstdint>

namespace flutter {

class TizenShell {
 public:
  static TizenShell& GetInstance() {
    static TizenShell instance;
    return instance;
  }

  TizenShell(const TizenShell&) = delete;
  TizenShell& operator=(const TizenShell&) = delete;

  void InitializeSoftkey(uint32_t window_id);

  bool IsSoftkeyShown() { return is_softkey_shown_; }

  void ShowSoftkey();

  void HideSoftkey();

 private:
  TizenShell();

  ~TizenShell();

  tzsh_h tizen_shell_ = nullptr;
  tzsh_softkey_h tizen_shell_softkey_ = nullptr;
  bool is_softkey_shown_ = true;
};

}  // namespace flutter

#endif  // EMBEDDER_TIZEN_SHELL_H_
