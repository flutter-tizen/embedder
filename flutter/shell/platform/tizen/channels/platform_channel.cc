// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform_channel.h"

#include <app.h>

#include <map>

#include "flutter/shell/platform/common/client_wrapper/include/flutter/method_result_functions.h"
#include "flutter/shell/platform/common/json_method_codec.h"
#include "flutter/shell/platform/tizen/channels/feedback_manager.h"
#ifdef COMMON_PROFILE
#include "flutter/shell/platform/tizen/channels/tizen_shell.h"
#endif
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/tizen_view.h"
#include "flutter/shell/platform/tizen/tizen_window.h"

namespace flutter {

namespace {

constexpr char kChannelName[] = "flutter/platform";

constexpr char kGetClipboardDataMethod[] = "Clipboard.getData";
constexpr char kSetClipboardDataMethod[] = "Clipboard.setData";
constexpr char kClipboardHasStringsMethod[] = "Clipboard.hasStrings";
constexpr char kExitApplicationMethod[] = "System.exitApplication";
constexpr char kRequestAppExitMethod[] = "System.requestAppExit";
constexpr char kInitializationCompleteMethod[] =
    "System.initializationComplete";
constexpr char kPlaySoundMethod[] = "SystemSound.play";
constexpr char kHapticFeedbackVibrateMethod[] = "HapticFeedback.vibrate";
constexpr char kSystemNavigatorPopMethod[] = "SystemNavigator.pop";
constexpr char kRestoreSystemUiOverlaysMethod[] =
    "SystemChrome.restoreSystemUIOverlays";
constexpr char kSetApplicationSwitcherDescriptionMethod[] =
    "SystemChrome.setApplicationSwitcherDescription";
constexpr char kSetEnabledSystemUiOverlaysMethod[] =
    "SystemChrome.setEnabledSystemUIOverlays";
constexpr char kSetPreferredOrientationsMethod[] =
    "SystemChrome.setPreferredOrientations";
constexpr char kSetSystemUIOverlayStyleMethod[] =
    "SystemChrome.setSystemUIOverlayStyle";
constexpr char kIsLiveTextInputAvailable[] =
    "LiveText.isLiveTextInputAvailable";

constexpr char kExitTypeKey[] = "type";
constexpr char kExitTypeCancelable[] = "cancelable";
constexpr char kExitTypeRequired[] = "required";
constexpr char kExitRequestError[] = "ExitApplication error";
constexpr char kExitResponseKey[] = "response";
constexpr char kExitResponseCancel[] = "cancel";
constexpr char kExitResponseExit[] = "exit";

constexpr char kTextKey[] = "text";
constexpr char kValueKey[] = "value";
constexpr char kTextPlainFormat[] = "text/plain";
constexpr char kUnknownClipboardFormatError[] =
    "Unknown clipboard format error";
constexpr char kUnknownClipboardError[] =
    "Unknown error during clipboard data retrieval";

constexpr char kSoundTypeClick[] = "SystemSoundType.click";
#ifdef COMMON_PROFILE
constexpr char kSystemUiOverlayBottom[] = "SystemUiOverlay.bottom";
#endif
constexpr char kPortraitUp[] = "DeviceOrientation.portraitUp";
constexpr char kPortraitDown[] = "DeviceOrientation.portraitDown";
constexpr char kLandscapeLeft[] = "DeviceOrientation.landscapeLeft";
constexpr char kLandscapeRight[] = "DeviceOrientation.landscapeRight";

}  // namespace

PlatformChannel::PlatformChannel(BinaryMessenger* messenger,
                                 TizenViewBase* view)
    : channel_(std::make_unique<MethodChannel<rapidjson::Document>>(
          messenger,
          kChannelName,
          &JsonMethodCodec::GetInstance())),
      view_(view) {
#ifdef CLIPBOARD_SUPPORT
  tizen_clipboard_ = std::make_unique<TizenClipboard>(view);
#endif

  channel_->SetMethodCallHandler(
      [this](const MethodCall<rapidjson::Document>& call,
             std::unique_ptr<MethodResult<rapidjson::Document>> result) {
        HandleMethodCall(call, std::move(result));
      });
}

PlatformChannel::~PlatformChannel() {}

void PlatformChannel::HandleMethodCall(
    const MethodCall<rapidjson::Document>& method_call,
    std::unique_ptr<MethodResult<rapidjson::Document>> result) {
  const std::string& method = method_call.method_name();
  const rapidjson::Document* arguments = method_call.arguments();

  if (method == kSystemNavigatorPopMethod) {
    SystemNavigatorPop();
    result->Success();
  } else if (method == kPlaySoundMethod) {
    PlaySystemSound(arguments[0].GetString());
    result->Success();
  } else if (method == kHapticFeedbackVibrateMethod) {
    std::string type;
    if (arguments->IsString()) {
      type = arguments[0].GetString();
    }
    HapticFeedbackVibrate(type);
    result->Success();
  } else if (method == kGetClipboardDataMethod) {
    // https://api.flutter.dev/flutter/services/Clipboard/kTextPlain-constant.html
    // The API only supports the plain text format.
    if (strcmp(arguments[0].GetString(), kTextPlainFormat) != 0) {
      result->Error(kUnknownClipboardFormatError,
                    "Clipboard API only supports text.");
      return;
    }
    auto* result_ptr = result.release();
    if (!GetClipboardData([result_ptr](std::optional<std::string> data) {
          if (!data.has_value()) {
            result_ptr->Error(kUnknownClipboardError, "Internal error.");
            delete result_ptr;
            return;
          }

          rapidjson::Document document;
          document.SetObject();
          rapidjson::Document::AllocatorType& allocator =
              document.GetAllocator();
          document.AddMember(rapidjson::Value(kTextKey, allocator),
                             rapidjson::Value(data.value(), allocator),
                             allocator);
          result_ptr->Success(document);
          delete result_ptr;
        })) {
      result_ptr->Error(kUnknownClipboardError, "Internal error.");
      delete result_ptr;
    };
  } else if (method == kSetClipboardDataMethod) {
    const rapidjson::Value& document = *arguments;
    auto iter = document.FindMember(kTextKey);
    if (iter == document.MemberEnd()) {
      result->Error(kUnknownClipboardError, "Invalid message format.");
      return;
    }
    SetClipboardData(iter->value.GetString());
    result->Success();
  } else if (method == kClipboardHasStringsMethod) {
    rapidjson::Document document;
    document.SetObject();
    rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
    document.AddMember(rapidjson::Value(kValueKey, allocator),
                       rapidjson::Value(ClipboardHasStrings()), allocator);
    result->Success(document);
  } else if (method == kExitApplicationMethod) {
    rapidjson::Value::ConstMemberIterator iter =
        arguments->FindMember(kExitTypeKey);
    if (iter == arguments->MemberEnd()) {
      result->Error(kExitRequestError, "Invalid application exit request.");
      return;
    }

    const std::string& exit_type = iter->value.GetString();
    rapidjson::Document document;
    document.SetObject();
    if (!initialization_complete_ || exit_type == kExitTypeRequired) {
      ui_app_exit();
      document.AddMember(kExitResponseKey, kExitResponseExit,
                         document.GetAllocator());
      result->Success(document);
    } else if (exit_type == kExitTypeCancelable) {
      RequestAppExit(exit_type);
      document.AddMember(kExitResponseKey, kExitResponseCancel,
                         document.GetAllocator());
      result->Success(document);
    } else {
      result->Error(kExitRequestError, "Invalid type.");
    }
  } else if (method == kInitializationCompleteMethod) {
    initialization_complete_ = true;
    result->Success();
  } else if (method == kRestoreSystemUiOverlaysMethod) {
    RestoreSystemUiOverlays();
    result->Success();
  } else if (method == kSetEnabledSystemUiOverlaysMethod) {
    const rapidjson::Document& list = arguments[0];
    std::vector<std::string> overlays;
    for (auto iter = list.Begin(); iter != list.End(); ++iter) {
      overlays.push_back(iter->GetString());
    }
    SetEnabledSystemUiOverlays(overlays);
    result->Success();
  } else if (method == kSetPreferredOrientationsMethod) {
    const rapidjson::Document& list = arguments[0];
    std::vector<std::string> orientations;
    for (auto iter = list.Begin(); iter != list.End(); ++iter) {
      orientations.push_back(iter->GetString());
    }
    SetPreferredOrientations(orientations);
    result->Success();
  } else if (method == kSetApplicationSwitcherDescriptionMethod ||
             method == kSetSystemUIOverlayStyleMethod ||
             method == kIsLiveTextInputAvailable) {
    // Not supported on Tizen. Ignore.
    result->Success();
  } else {
    FT_LOG(Info) << "Unimplemented method: " << method;
    result->NotImplemented();
  }
}

void PlatformChannel::SystemNavigatorPop() {
  if (auto* view = dynamic_cast<TizenView*>(view_)) {
    view->SetFocus(false);
  } else {
    ui_app_exit();
  }
}

void PlatformChannel::PlaySystemSound(const std::string& sound_type) {
  if (sound_type == kSoundTypeClick) {
    FeedbackManager::GetInstance().PlayTapSound();
  } else {
    FeedbackManager::GetInstance().PlaySound();
  }
}

void PlatformChannel::HapticFeedbackVibrate(const std::string& feedback_type) {
  FeedbackManager::GetInstance().Vibrate();
}

bool PlatformChannel::GetClipboardData(ClipboardCallback on_data) {
#ifdef CLIPBOARD_SUPPORT
  return tizen_clipboard_->GetData(std::move(on_data));
#else
  on_data(clipboard_);
  return true;
#endif
}

void PlatformChannel::SetClipboardData(const std::string& data) {
#ifdef CLIPBOARD_SUPPORT
  tizen_clipboard_->SetData(data);
#else
  clipboard_ = data;
#endif
}

bool PlatformChannel::ClipboardHasStrings() {
#ifdef CLIPBOARD_SUPPORT
  return tizen_clipboard_->HasStrings();
#else
  return !clipboard_.empty();
#endif
}

void PlatformChannel::RestoreSystemUiOverlays() {
  if (auto* window = dynamic_cast<TizenWindow*>(view_)) {
#ifdef COMMON_PROFILE
    auto& shell = TizenShell::GetInstance();
    shell.InitializeSoftkey(window->GetWindowId());

    if (shell.IsSoftkeyShown()) {
      shell.ShowSoftkey();
    } else {
      shell.HideSoftkey();
    }
#endif
  }
}

void PlatformChannel::RequestAppExit(const std::string exit_type) {
  if (!initialization_complete_) {
    ui_app_exit();
    return;
  }
  auto callback = std::make_unique<MethodResultFunctions<rapidjson::Document>>(
      [this](const rapidjson::Document* response) {
        RequestAppExitSuccess(response);
      },
      nullptr, nullptr);
  auto args = std::make_unique<rapidjson::Document>();
  args->SetObject();
  args->AddMember(kExitTypeKey, exit_type, args->GetAllocator());
  channel_->InvokeMethod(kRequestAppExitMethod, std::move(args),
                         std::move(callback));
}

void PlatformChannel::RequestAppExitSuccess(const rapidjson::Document* result) {
  rapidjson::Value::ConstMemberIterator itr =
      result->FindMember(kExitResponseKey);
  if (itr == result->MemberEnd() || !itr->value.IsString()) {
    FT_LOG(Error) << "Application request response did not contain a valid "
                     "response value";
    return;
  }
  const std::string& exit_type = itr->value.GetString();
  if (exit_type == kExitResponseExit) {
    ui_app_exit();
  }
}

void PlatformChannel::SetEnabledSystemUiOverlays(
    const std::vector<std::string>& overlays) {
  if (auto* window = dynamic_cast<TizenWindow*>(view_)) {
#ifdef COMMON_PROFILE
    auto& shell = TizenShell::GetInstance();
    shell.InitializeSoftkey(window->GetWindowId());

    if (std::find(overlays.begin(), overlays.end(), kSystemUiOverlayBottom) !=
        overlays.end()) {
      shell.ShowSoftkey();
    } else {
      shell.HideSoftkey();
    }
#endif
  }
}

void PlatformChannel::SetPreferredOrientations(
    const std::vector<std::string>& orientations) {
  if (auto* window = dynamic_cast<TizenWindow*>(view_)) {
    std::map<std::string, int> orientation_mapping;
    TizenGeometry geometry = window->GetGeometry();
    if (geometry.width > geometry.height) {
      orientation_mapping = {
          {kLandscapeLeft, 0},
          {kPortraitDown, 90},
          {kLandscapeRight, 180},
          {kPortraitUp, 270},
      };
    } else {
      orientation_mapping = {
          {kPortraitUp, 0},
          {kLandscapeLeft, 90},
          {kPortraitDown, 180},
          {kLandscapeRight, 270},
      };
    }
    std::vector<int> rotations;
    for (const auto& orientation : orientations) {
      rotations.push_back(orientation_mapping.at(orientation));
    }
    if (rotations.empty()) {
      // The empty list causes the application to defer to the operating system
      // default.
      rotations = {0, 90, 180, 270};
    }
    window->SetPreferredOrientations(rotations);
  }
}

}  // namespace flutter
