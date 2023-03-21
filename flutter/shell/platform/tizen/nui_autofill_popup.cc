// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/nui_autofill_popup.h"

#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/public-api/controls/text-controls/text-label.h>

namespace flutter {

bool NuiAutofillPopup::Touched(Dali::Actor actor,
                               const Dali::TouchEvent& event) {
  const Dali::PointState::Type state = event.GetState(0);
  if (Dali::PointState::DOWN == state) {
    std::string text =
        actor.GetProperty(Dali::Actor::Property::NAME).Get<std::string>();
    on_commit_(text);
    popup_.SetDisplayState(Dali::Toolkit::Popup::HIDDEN);
  }
  return true;
}

void NuiAutofillPopup::Hidden() {
  // TODO(Swanseo0): There is a phenomenon where white traces remain for a
  // while when popup disappears.
  popup_.Unparent();
  popup_.Reset();
}

void NuiAutofillPopup::OutsideTouched() {
  popup_.SetDisplayState(Dali::Toolkit::Popup::HIDDEN);
}

Dali::Toolkit::TableView NuiAutofillPopup::MakeContent(
    const std::vector<std::unique_ptr<AutofillItem>>& items) {
  Dali::Toolkit::TableView content =
      Dali::Toolkit::TableView::New(items.size(), 1);
  content.SetResizePolicy(Dali::ResizePolicy::FILL_TO_PARENT,
                          Dali::Dimension::ALL_DIMENSIONS);
  content.SetProperty(Dali::Actor::Property::PADDING,
                      Dali::Vector4(10, 10, 0, 0));
  for (uint32_t i = 0; i < items.size(); ++i) {
    Dali::Toolkit::TextLabel label =
        Dali::Toolkit::TextLabel::New(items[i]->label);
    label.SetProperty(Dali::Actor::Property::NAME, items[i]->value);
    label.SetResizePolicy(Dali::ResizePolicy::DIMENSION_DEPENDENCY,
                          Dali::Dimension::HEIGHT);
    label.SetProperty(Dali::Toolkit::TextLabel::Property::TEXT_COLOR,
                      Dali::Color::WHITE_SMOKE);
    label.SetProperty(Dali::Toolkit::TextLabel::Property::POINT_SIZE, 7.0f);
    label.TouchedSignal().Connect(this, &NuiAutofillPopup::Touched);
    content.AddChild(label, Dali::Toolkit::TableView::CellPosition(i, 0));
    content.SetFitHeight(i);
  }
  return content;
}

void NuiAutofillPopup::Prepare(
    const std::vector<std::unique_ptr<AutofillItem>>& items) {
  popup_ = Dali::Toolkit::Popup::New();
  popup_.SetProperty(Dali::Actor::Property::NAME, "popup");
  popup_.SetProperty(Dali::Actor::Property::PARENT_ORIGIN,
                     Dali::ParentOrigin::TOP_LEFT);
  popup_.SetProperty(Dali::Actor::Property::ANCHOR_POINT,
                     Dali::AnchorPoint::TOP_LEFT);
  popup_.SetProperty(Dali::Toolkit::Popup::Property::TAIL_VISIBILITY, false);
  popup_.SetBackgroundColor(Dali::Color::WHITE_SMOKE);
  popup_.OutsideTouchedSignal().Connect(this,
                                        &NuiAutofillPopup::OutsideTouched);
  popup_.HiddenSignal().Connect(this, &NuiAutofillPopup::Hidden);
  popup_.SetProperty(Dali::Toolkit::Popup::Property::BACKING_ENABLED, false);
  popup_.SetProperty(Dali::Toolkit::Popup::Property::AUTO_HIDE_DELAY, 2500);
  popup_.SetProperty(Dali::Actor::Property::SIZE,
                     Dali::Vector2(140.0f, 35.0f * items.size()));

  Dali::Toolkit::TableView content = MakeContent(items);
  popup_.SetContent(content);
}

void NuiAutofillPopup::Show(Dali::Actor* actor, double_t x, double_t y) {
  const std::vector<std::unique_ptr<AutofillItem>>& items =
      TizenAutofill::GetInstance().GetResponseItems();
  if (items.empty()) {
    return;
  }

  Prepare(items);

  popup_.SetProperty(Dali::Actor::Property::POSITION,
                     Dali::Vector3(x, y, 0.5f));
  popup_.SetDisplayState(Dali::Toolkit::Popup::SHOWN);
  actor->Add(popup_);
}

}  // namespace flutter
