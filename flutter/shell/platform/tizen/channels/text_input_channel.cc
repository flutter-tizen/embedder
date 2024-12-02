// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "text_input_channel.h"

#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/textinput";

constexpr char kSetEditingStateMethod[] = "TextInput.setEditingState";
constexpr char kClearClientMethod[] = "TextInput.clearClient";
constexpr char kSetClientMethod[] = "TextInput.setClient";
constexpr char kShowMethod[] = "TextInput.show";
constexpr char kHideMethod[] = "TextInput.hide";
constexpr char kNoneInputType[] = "TextInputType.none";
constexpr char kMultilineInputType[] = "TextInputType.multiline";
constexpr char kUpdateEditingStateMethod[] =
    "TextInputClient.updateEditingState";
constexpr char kPerformActionMethod[] = "TextInputClient.performAction";
constexpr char kSetPlatformViewClient[] = "TextInput.setPlatformViewClient";
constexpr char kTextCapitalization[] = "textCapitalization";
constexpr char kTextInputAction[] = "inputAction";
constexpr char kTextInputType[] = "inputType";
constexpr char kTextInputTypeName[] = "name";
constexpr char kTextInputTypeSigned[] = "signed";
constexpr char kTextInputTypeDecimal[] = "decimal";
constexpr char kComposingBaseKey[] = "composingBase";
constexpr char kComposingExtentKey[] = "composingExtent";
constexpr char kSelectionAffinityKey[] = "selectionAffinity";
constexpr char kAffinityDownstream[] = "TextAffinity.downstream";
constexpr char kSelectionBaseKey[] = "selectionBase";
constexpr char kSelectionExtentKey[] = "selectionExtent";
constexpr char kSelectionIsDirectionalKey[] = "selectionIsDirectional";
constexpr char kTextKey[] = "text";
constexpr char kBadArgumentError[] = "Bad Arguments";
constexpr char kInternalConsistencyError[] = "Internal Consistency Error";

bool IsAsciiPrintableKey(char ch) {
  return ch >= 32 && ch <= 126;
}

}  // namespace

TextInputChannel::TextInputChannel(
    BinaryMessenger* messenger,
    TizenInputMethodContext* input_method_context)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())),
      input_method_context_(input_method_context) {
  channel_->SetMethodCallHandler(
      [this](const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

TextInputChannel::~TextInputChannel() {}

void TextInputChannel::OnComposeBegin() {
  if (active_model_ == nullptr) {
    return;
  }
  active_model_->BeginComposing();
}

void TextInputChannel::OnComposeChange(const std::string& str, int cursor_pos) {
  if (active_model_ == nullptr) {
    return;
  }
  if (str == "") {
    // Enter pre-edit end stage.
    return;
  }
  active_model_->UpdateComposingText(str);
  SendStateUpdate();
}

void TextInputChannel::OnComposeEnd() {
  if (active_model_ == nullptr) {
    return;
  }
  // Delete preedit-string, it will be committed.
  int count = active_model_->composing_range().extent() -
              active_model_->composing_range().base();

  active_model_->CommitComposing();
  active_model_->EndComposing();
  active_model_->DeleteSurrounding(-count, count);
  SendStateUpdate();
}

void TextInputChannel::OnCommit(const std::string& str) {
  if (active_model_ == nullptr) {
    return;
  }
  active_model_->AddText(str);
  if (active_model_->composing()) {
    active_model_->CommitComposing();
    active_model_->EndComposing();
  }
  SendStateUpdate();
}

bool TextInputChannel::SendKey(const char* key,
                               const char* string,
                               const char* compose,
                               uint32_t modifiers,
                               uint32_t scan_code,
                               bool is_down) {
  if (active_model_ == nullptr) {
    return false;
  }

  if (is_down) {
    return HandleKey(key, string, modifiers);
  }

  return false;
}

void TextInputChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& method_call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const std::string& method = method_call.method_name();

  if (method.compare(kShowMethod) == 0) {
    if (input_type_ != kNoneInputType) {
      input_method_context_->ShowInputPanel();
    }
  } else if (method.compare(kHideMethod) == 0) {
    input_method_context_->HideInputPanel();
    input_method_context_->ResetInputMethodContext();
  } else if (method.compare(kSetPlatformViewClient) == 0) {
    result->NotImplemented();
    return;
  } else if (method.compare(kClearClientMethod) == 0) {
    active_model_ = nullptr;
  } else if (method.compare(kSetClientMethod) == 0) {
    if (!method_call.arguments() || method_call.arguments()->IsNull()) {
      result->Error(kBadArgumentError, "Method invoked without args.");
      return;
    }
    const rapidjson::Document& args = *method_call.arguments();

    // TODO(awdavies): There's quite a wealth of arguments supplied with this
    // method, and they should be inspected/used.
    const rapidjson::Value& client_id_json = args[0];
    const rapidjson::Value& client_config = args[1];
    if (client_id_json.IsNull()) {
      result->Error(kBadArgumentError, "Could not set client, ID is null.");
      return;
    }
    if (client_config.IsNull()) {
      result->Error(kBadArgumentError,
                    "Could not set client, missing arguments.");
    }

    client_id_ = client_id_json.GetInt();
    input_action_ = "";
    auto input_action_iter = client_config.FindMember(kTextInputAction);
    if (input_action_iter != client_config.MemberEnd() &&
        input_action_iter->value.IsString()) {
      input_action_ = input_action_iter->value.GetString();
    }

    text_capitalization_ = "";
    auto text_capitalization_iter =
        client_config.FindMember(kTextCapitalization);
    if (text_capitalization_iter != client_config.MemberEnd() &&
        text_capitalization_iter->value.IsString()) {
      text_capitalization_ = text_capitalization_iter->value.GetString();
      input_method_context_->SetAutocapitalType(text_capitalization_);
    }

    input_type_ = "";
    auto input_type_info_iter = client_config.FindMember(kTextInputType);
    if (input_type_info_iter != client_config.MemberEnd() &&
        input_type_info_iter->value.IsObject()) {
      auto input_type_iter =
          input_type_info_iter->value.FindMember(kTextInputTypeName);
      if (input_type_iter != input_type_info_iter->value.MemberEnd() &&
          input_type_iter->value.IsString()) {
        input_type_ = input_type_iter->value.GetString();
        bool is_signed = false;
        auto is_signed_iter =
            input_type_info_iter->value.FindMember(kTextInputTypeSigned);
        if (is_signed_iter != input_type_info_iter->value.MemberEnd() &&
            is_signed_iter->value.IsBool()) {
          is_signed = is_signed_iter->value.GetBool();
        }
        bool is_decimal = false;
        auto is_decimal_iter =
            input_type_info_iter->value.FindMember(kTextInputTypeDecimal);
        if (is_decimal_iter != input_type_info_iter->value.MemberEnd() &&
            is_decimal_iter->value.IsBool()) {
          is_decimal = is_decimal_iter->value.GetBool();
        }
        input_method_context_->SetInputPanelLayout(input_type_);
        input_method_context_->SetInputPanelLayoutVariation(is_signed,
                                                            is_decimal);
        // The panel should be closed and reopened to fully apply the layout
        // change. See https://github.com/flutter-tizen/engine/pull/194.
        input_method_context_->HideInputPanel();
        if (input_type_ != kNoneInputType) {
          input_method_context_->ShowInputPanel();
        }
      }
    }

    active_model_ = std::make_unique<TextInputModel>();
  } else if (method.compare(kSetEditingStateMethod) == 0) {
    input_method_context_->ResetInputMethodContext();
    if (!method_call.arguments() || method_call.arguments()->IsNull()) {
      result->Error(kBadArgumentError, "Method invoked without args.");
      return;
    }

    const rapidjson::Document& args = *method_call.arguments();

    if (active_model_ == nullptr) {
      result->Error(
          kInternalConsistencyError,
          "Set editing state has been invoked, but no client is set.");
      return;
    }

    auto text = args.FindMember(kTextKey);
    if (text == args.MemberEnd() || text->value.IsNull()) {
      result->Error(kBadArgumentError,
                    "Set editing state has been invoked, but without text.");
      return;
    }

    auto selection_base = args.FindMember(kSelectionBaseKey);
    auto selection_extent = args.FindMember(kSelectionExtentKey);
    if (selection_base == args.MemberEnd() || selection_base->value.IsNull() ||
        selection_extent == args.MemberEnd() ||
        selection_extent->value.IsNull()) {
      result->Error(kInternalConsistencyError,
                    "Selection base/extent values invalid.");
      return;
    }
    int selection_base_value = selection_base->value.GetInt();
    int selection_extent_value = selection_extent->value.GetInt();

    active_model_->SetText(text->value.GetString());
    active_model_->SetSelection(
        TextRange(selection_base_value, selection_extent_value));

    auto composing_base = args.FindMember(kComposingBaseKey);
    auto composing_extent = args.FindMember(kComposingBaseKey);
    int composing_base_value = composing_base != args.MemberEnd()
                                   ? composing_base->value.GetInt()
                                   : -1;
    int composing_extent_value = composing_extent != args.MemberEnd()
                                     ? composing_extent->value.GetInt()
                                     : -1;

    if (composing_base_value == -1 && composing_extent_value == -1) {
      active_model_->EndComposing();
    } else {
      size_t composing_start = static_cast<size_t>(
          std::min(composing_base_value, composing_extent_value));
      size_t cursor_offset = selection_base_value - composing_start;

      active_model_->SetComposingRange(
          TextRange(static_cast<size_t>(composing_base_value),
                    static_cast<size_t>(composing_extent_value)),
          cursor_offset);
    }
    SendStateUpdate();
  } else {
    result->NotImplemented();
    return;
  }
  // All error conditions return early, so if nothing has gone wrong indicate
  // success.
  result->Success();
}

void TextInputChannel::SendStateUpdate() {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  rapidjson::MemoryPoolAllocator<>& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);

  TextRange selection = active_model_->selection();
  rapidjson::Value editing_state(rapidjson::kObjectType);
  int composing_base =
      active_model_->composing() ? active_model_->composing_range().base() : -1;
  int composing_extent = active_model_->composing()
                             ? active_model_->composing_range().extent()
                             : -1;

  editing_state.AddMember(kComposingBaseKey, composing_base, allocator);
  editing_state.AddMember(kComposingExtentKey, composing_extent, allocator);
  editing_state.AddMember(kSelectionAffinityKey, kAffinityDownstream,
                          allocator);
  editing_state.AddMember(kSelectionBaseKey, selection.base(), allocator);
  editing_state.AddMember(kSelectionExtentKey, selection.extent(), allocator);
  editing_state.AddMember(kSelectionIsDirectionalKey, false, allocator);
  editing_state.AddMember(
      kTextKey, rapidjson::Value(active_model_->GetText(), allocator).Move(),
      allocator);
  args->PushBack(editing_state, allocator);

  channel_->InvokeMethod(kUpdateEditingStateMethod, std::move(args));
}

bool TextInputChannel::HandleKey(const char* key,
                                 const char* string,
                                 uint32_t modifires) {
  bool needs_update = false;
  std::string key_str = key;

  if (string && strlen(string) == 1 && IsAsciiPrintableKey(string[0])) {
    // This is a fallback for printable keys not handled by IMF.
    active_model_->AddCodePoint(string[0]);
    needs_update = true;
  } else if (key_str == "Return") {
    EnterPressed();
    return true;
#ifdef TV_PROFILE
  } else if (key_str == "Select") {
    SelectPressed();
    return true;
#endif
  } else {
    FT_LOG(Info) << "Key[" << key << "] is unhandled.";
    return false;
  }

  if (needs_update) {
    SendStateUpdate();
  }
  return true;
}

void TextInputChannel::EnterPressed() {
  if (input_type_ == kMultilineInputType) {
    active_model_->AddCodePoint('\n');
    SendStateUpdate();
  }
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  rapidjson::MemoryPoolAllocator<>& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);
  args->PushBack(rapidjson::Value(input_action_, allocator).Move(), allocator);

  channel_->InvokeMethod(kPerformActionMethod, std::move(args));
}

#ifdef TV_PROFILE
void TextInputChannel::SelectPressed() {
  auto args = std::make_unique<rapidjson::Document>(rapidjson::kArrayType);
  rapidjson::MemoryPoolAllocator<>& allocator = args->GetAllocator();
  args->PushBack(client_id_, allocator);
  args->PushBack(rapidjson::Value(input_action_, allocator).Move(), allocator);

  channel_->InvokeMethod(kPerformActionMethod, std::move(args));
}
#endif

}  // namespace flutter
