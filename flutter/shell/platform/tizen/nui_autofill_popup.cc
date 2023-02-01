#include "flutter/shell/platform/tizen/nui_autofill_popup.h"

#include <dali-toolkit/dali-toolkit.h>
#include <dali-toolkit/devel-api/controls/table-view/table-view.h>
#include <dali-toolkit/public-api/controls/text-controls/text-label.h>

#include "flutter/shell/platform/tizen/tizen_autofill.h"

#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {
bool NuiAutofillPopup::OnTouch(Dali::Actor actor,
                               const Dali::TouchEvent& event) {
  const Dali::PointState::Type state = event.GetState(0);
  if (Dali::PointState::DOWN == state) {
    auto t = actor.GetProperty(Dali::Actor::Property::NAME).Get<std::string>();
    on_commit_(t);
    OnHide();
  }
  return true;
}

void NuiAutofillPopup::OnHide() {
  // TODO : There is a phenomenon where white traces remain for a while when
  // popup disappears.
  popup_.SetDisplayState(Dali::Toolkit::Popup::HIDDEN);
}

void NuiAutofillPopup::OnHidden() {
  popup_.Unparent();
  popup_.Reset();
}

void NuiAutofillPopup::PrepareAutofill() {
  popup_ = Dali::Toolkit::Popup::New();
  popup_.SetProperty(Dali::Actor::Property::NAME, "popup");
  popup_.SetProperty(Dali::Actor::Property::PARENT_ORIGIN,
                     Dali::ParentOrigin::TOP_LEFT);
  popup_.SetProperty(Dali::Actor::Property::ANCHOR_POINT,
                     Dali::AnchorPoint::TOP_LEFT);
  popup_.SetProperty(Dali::Toolkit::Popup::Property::TAIL_VISIBILITY, false);
  popup_.SetBackgroundColor(Dali::Color::WHITE_SMOKE);
  popup_.OutsideTouchedSignal().Connect(this, &NuiAutofillPopup::OnHide);
  popup_.HiddenSignal().Connect(this, &NuiAutofillPopup::OnHidden);
  popup_.SetProperty(Dali::Toolkit::Popup::Property::BACKING_ENABLED, false);
  popup_.SetProperty(Dali::Toolkit::Popup::Property::AUTO_HIDE_DELAY, 2500);
}

void NuiAutofillPopup::PopupAutofill(Dali::Actor* actor) {
  const auto& items = TizenAutofill::GetInstance().GetAutofillItems();
  if (items.size() > 0) {
    PrepareAutofill();
    Dali::Toolkit::TableView content =
        Dali::Toolkit::TableView::New(items.size(), 1);
    // content.SetCellPadding(Dali::Size(10.0f, 0));
    content.SetResizePolicy(Dali::ResizePolicy::FILL_TO_PARENT,
                            Dali::Dimension::ALL_DIMENSIONS);
    content.SetProperty(Dali::Actor::Property::PADDING,
                        Dali::Vector4(10, 10, 0, 0));
    for (uint32_t i = 0; i < items.size(); ++i) {
      auto label = Dali::Toolkit::TextLabel::New(items[i]->label_);
      label.SetProperty(Dali::Actor::Property::NAME, items[i]->value_);
      label.SetResizePolicy(Dali::ResizePolicy::DIMENSION_DEPENDENCY,
                            Dali::Dimension::HEIGHT);
      label.SetProperty(Dali::Toolkit::TextLabel::Property::TEXT_COLOR,
                        Dali::Color::WHITE_SMOKE);
      label.SetProperty(Dali::Toolkit::TextLabel::Property::POINT_SIZE, 7.0f);
      label.TouchedSignal().Connect(this, &NuiAutofillPopup::OnTouch);
      content.AddChild(label, Dali::Toolkit::TableView::CellPosition(i, 0));
      content.SetFitHeight(i);
    }
    popup_.SetProperty(Dali::Actor::Property::SIZE,
                       Dali::Vector2(140.0f, 35.0f * items.size()));
    popup_.SetContent(content);
    popup_.SetDisplayState(Dali::Toolkit::Popup::SHOWN);
    actor->Add(popup_);
  }
}

}  // namespace flutter
