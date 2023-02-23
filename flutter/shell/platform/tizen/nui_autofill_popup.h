// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_NUI_AUTOFILL_POPUP_H_
#define EMBEDDER_NUI_AUTOFILL_POPUP_H_

#include <dali-toolkit/devel-api/controls/popup/popup.h>
#include <dali-toolkit/devel-api/controls/table-view/table-view.h>

#include <functional>

#include "flutter/shell/platform/tizen/tizen_autofill.h"

using OnCommit = std::function<void(const std::string& str)>;

namespace flutter {

class NuiAutofillPopup : public Dali::ConnectionTracker {
 public:
  void Show(Dali::Actor* actor);

  void SetOnCommit(OnCommit callback) { on_commit_ = callback; }

 private:
  void Prepare(const std::vector<std::unique_ptr<AutofillItem>>& items);

  void Hidden();

  void OutsideTouched();

  bool Touched(Dali::Actor actor, const Dali::TouchEvent& event);

  Dali::Toolkit::TableView MakeContent(
      const std::vector<std::unique_ptr<AutofillItem>>& items);

  Dali::Toolkit::Popup popup_;
  OnCommit on_commit_;
};

}  // namespace flutter

#endif  // EMBEDDER_NUI_AUTOFILL_POPUP_H_
