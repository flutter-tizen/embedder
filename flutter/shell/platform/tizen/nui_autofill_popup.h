// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_NUI_AUTOFILL_POPUP_H_
#define EMBEDDER_NUI_AUTOFILL_POPUP_H_

#include <dali-toolkit/devel-api/controls/popup/popup.h>

#include <functional>

namespace flutter {

using OnCommit = std::function<void(std::string str)>;
using OnRendering = std::function<void()>;

class NuiAutofillPopup : public Dali::ConnectionTracker {
 public:
  void Hide();

  void Hidden();

  void OutsideTouched();

  void PrepareAutofill();

  void PopupAutofill(Dali::Actor* actor);

  void SetOnCommit(OnCommit callback) { on_commit_ = callback; }

  bool Touched(Dali::Actor actor, const Dali::TouchEvent& event);

 private:
  Dali::Toolkit::Popup popup_;
  OnCommit on_commit_;
};

}  // namespace flutter

#endif
