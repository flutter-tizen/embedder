// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_view_nui.h"

#include <dali/devel-api/common/stage.h>

#include <string>

#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_view_event_handler_delegate.h"

namespace flutter {

TizenViewNui::TizenViewNui(int32_t width,
                           int32_t height,
                           Dali::Toolkit::ImageView* image_view,
                           Dali::NativeImageSourceQueuePtr native_image_queue,
                           int32_t default_window_id)
    : TizenView(width, height),
      image_view_(image_view),
      native_image_queue_(native_image_queue),
      default_window_id_(default_window_id) {
  RegisterEventHandlers();
  PrepareInputMethod();
  Show();
}

TizenViewNui::~TizenViewNui() {
  UnregisterEventHandlers();
}

void TizenViewNui::RegisterEventHandlers() {
  rendering_callback_ = std::make_unique<Dali::EventThreadCallback>(
      Dali::MakeCallback(this, &TizenViewNui::RenderOnce));
}

void TizenViewNui::UnregisterEventHandlers() {
  rendering_callback_.release();
}

TizenGeometry TizenViewNui::GetGeometry() {
  Dali::Vector2 size = image_view_->GetProperty(Dali::Actor::Property::SIZE)
                           .Get<Dali::Vector2>();
  TizenGeometry result = {0, 0, static_cast<int32_t>(size.width),
                          static_cast<int32_t>(size.height)};
  return result;
}

bool TizenViewNui::SetGeometry(TizenGeometry geometry) {
  view_delegate_->OnResize(0, 0, geometry.width, geometry.height);

  image_view_->SetProperty(Dali::Actor::Property::SIZE,
                           Dali::Vector2(geometry.width, geometry.height));

  native_image_queue_->SetSize(geometry.width, geometry.height);
  return true;
}

int32_t TizenViewNui::GetDpi() {
  return Dali::Stage::GetCurrent().GetDpi().width;
}

uintptr_t TizenViewNui::GetWindowId() {
  return default_window_id_;
}

void TizenViewNui::Show() {
  // Do nothing.
}

void TizenViewNui::RequestRendering() {
  rendering_callback_->Trigger();
}

void TizenViewNui::OnKey(const char* device_name,
                         uint32_t device_class,
                         uint32_t device_subclass,
                         const char* key,
                         const char* string,
                         const char* compose,
                         uint32_t modifiers,
                         uint32_t scan_code,
                         size_t timestamp,
                         bool is_down) {
  bool handled = false;

  if (input_method_context_->IsInputPanelShown()) {
    handled = input_method_context_->HandleNuiKeyEvent(
        device_name, device_class, device_subclass, key, string, modifiers,
        scan_code, timestamp, is_down);
  }

  if (!handled) {
    view_delegate_->OnKey(key, string, compose, modifiers, scan_code,
                          device_name, is_down);
  }
}

void TizenViewNui::UpdateFlutterCursor(const std::string& kind) {
  FT_LOG(Info) << "UpdateFlutterCursor is not supported.";
}

void TizenViewNui::PrepareInputMethod() {
  input_method_context_ =
      std::make_unique<TizenInputMethodContext>(GetWindowId());

  // Set input method callbacks.
  input_method_context_->SetOnPreeditStart(
      [this]() { view_delegate_->OnComposeBegin(); });
  input_method_context_->SetOnPreeditChanged(
      [this](std::string str, int cursor_pos) {
        view_delegate_->OnComposeChange(str, cursor_pos);
      });
  input_method_context_->SetOnPreeditEnd(
      [this]() { view_delegate_->OnComposeEnd(); });
  input_method_context_->SetOnCommit(
      [this](std::string str) { view_delegate_->OnCommit(str); });
}

void TizenViewNui::RenderOnce() {
  Dali::Stage::GetCurrent().KeepRendering(0.0f);
}

}  // namespace flutter
