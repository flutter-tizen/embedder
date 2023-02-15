// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_NUI_AUTOFILL_POPUP_H_
#define EMBEDDER_NUI_AUTOFILL_POPUP_H_

#include <dali-toolkit/devel-api/controls/popup/popup.h>

#include <functional>

namespace flutter {

class NuiAutofillPopup : public Dali::ConnectionTracker {
 public:
  void Show(Dali::Actor* actor);

  void SetOnCommit(std::function<void(const std::string&)> callback) {
    on_commit_ = callback;
  }

 private:
  void Prepare();

  void Hidden();

  void OutsideTouched();

  bool Touched(Dali::Actor actor, const Dali::TouchEvent& event);

  Dali::Toolkit::Popup popup_;
  std::function<void(const std::string&)> on_commit_;
};

}  // namespace flutter

#endif
