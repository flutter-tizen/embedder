// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/flutter_tizen.h"

#include "flutter/shell/platform/common/client_wrapper/include/flutter/plugin_registrar.h"
#include "flutter/shell/platform/common/client_wrapper/include/flutter/standard_message_codec.h"
#include "flutter/shell/platform/common/incoming_message_dispatcher.h"
#include "flutter/shell/platform/tizen/flutter_project_bundle.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/public/flutter_platform_view.h"
#include "flutter/shell/platform/tizen/tizen_view.h"
#ifdef NUI_SUPPORT
#include "flutter/shell/platform/tizen/tizen_view_nui.h"
#endif
#include "flutter/shell/platform/tizen/tizen_window.h"
#ifndef WEARABLE_PROFILE
#include "flutter/shell/platform/tizen/tizen_window_ecore_wl2.h"
#endif
#include "flutter/shell/platform/tizen/tizen_window_elementary.h"

namespace {

// Returns the engine corresponding to the given opaque API handle.
flutter::FlutterTizenEngine* EngineFromHandle(FlutterDesktopEngineRef ref) {
  return reinterpret_cast<flutter::FlutterTizenEngine*>(ref);
}

// Returns the opaque API handle for the given engine instance.
FlutterDesktopEngineRef HandleForEngine(flutter::FlutterTizenEngine* engine) {
  return reinterpret_cast<FlutterDesktopEngineRef>(engine);
}

// Returns the view corresponding to the given opaque API handle.
flutter::FlutterTizenView* ViewFromHandle(FlutterDesktopViewRef view) {
  return reinterpret_cast<flutter::FlutterTizenView*>(view);
}

// Returns the texture registrar corresponding to the given opaque API handle.
flutter::FlutterTizenTextureRegistrar* TextureRegistrarFromHandle(
    FlutterDesktopTextureRegistrarRef ref) {
  return reinterpret_cast<flutter::FlutterTizenTextureRegistrar*>(ref);
}

// Returns the opaque API handle for the given texture registrar instance.
FlutterDesktopTextureRegistrarRef HandleForTextureRegistrar(
    flutter::FlutterTizenTextureRegistrar* registrar) {
  return reinterpret_cast<FlutterDesktopTextureRegistrarRef>(registrar);
}

FlutterDesktopViewRef HandleForView(flutter::FlutterTizenView* view) {
  return reinterpret_cast<FlutterDesktopViewRef>(view);
}

}  // namespace

FlutterDesktopEngineRef FlutterDesktopEngineCreate(
    const FlutterDesktopEngineProperties& engine_properties) {
  flutter::FlutterProjectBundle project(engine_properties);
  if (project.HasArgument("--verbose-logging")) {
    flutter::Logger::SetLoggingLevel(flutter::kLogLevelDebug);
  }
  std::string logging_port;
  if (project.GetArgumentValue("--tizen-logging-port", &logging_port)) {
    flutter::Logger::SetLoggingPort(std::stoi(logging_port));
  }
  flutter::Logger::Start();

  auto engine = std::make_unique<flutter::FlutterTizenEngine>(project);
  return HandleForEngine(engine.release());
}

bool FlutterDesktopEngineRun(const FlutterDesktopEngineRef engine) {
  return EngineFromHandle(engine)->RunEngine();
}

void FlutterDesktopEngineShutdown(FlutterDesktopEngineRef engine_ref) {
  flutter::Logger::Stop();

  flutter::FlutterTizenEngine* engine = EngineFromHandle(engine_ref);
  engine->StopEngine();
  delete engine;
}

FlutterDesktopViewRef FlutterDesktopPluginRegistrarGetView(
    FlutterDesktopPluginRegistrarRef registrar) {
  return HandleForView(registrar->engine->view());
}

void FlutterDesktopPluginRegistrarEnableInputBlocking(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* channel) {
  registrar->engine->message_dispatcher()->EnableInputBlockingForChannel(
      channel);
}

FlutterDesktopPluginRegistrarRef FlutterDesktopEngineGetPluginRegistrar(
    FlutterDesktopEngineRef engine,
    const char* plugin_name) {
  // Currently, one registrar acts as the registrar for all plugins, so the
  // name is ignored. It is part of the API to reduce churn in the future when
  // aligning more closely with the Flutter registrar system.
  return EngineFromHandle(engine)->plugin_registrar();
}

FlutterDesktopMessengerRef FlutterDesktopEngineGetMessenger(
    FlutterDesktopEngineRef engine) {
  return EngineFromHandle(engine)->messenger();
}

FlutterDesktopMessengerRef FlutterDesktopPluginRegistrarGetMessenger(
    FlutterDesktopPluginRegistrarRef registrar) {
  return registrar->engine->messenger();
}

void FlutterDesktopPluginRegistrarSetDestructionHandler(
    FlutterDesktopPluginRegistrarRef registrar,
    FlutterDesktopOnPluginRegistrarDestroyed callback) {
  registrar->engine->AddPluginRegistrarDestructionCallback(callback, registrar);
}

bool FlutterDesktopMessengerSend(FlutterDesktopMessengerRef messenger,
                                 const char* channel,
                                 const uint8_t* message,
                                 const size_t message_size) {
  return FlutterDesktopMessengerSendWithReply(messenger, channel, message,
                                              message_size, nullptr, nullptr);
}

bool FlutterDesktopMessengerSendWithReply(FlutterDesktopMessengerRef messenger,
                                          const char* channel,
                                          const uint8_t* message,
                                          const size_t message_size,
                                          const FlutterDesktopBinaryReply reply,
                                          void* user_data) {
  return messenger->engine->SendPlatformMessage(channel, message, message_size,
                                                reply, user_data);
}

void FlutterDesktopMessengerSendResponse(
    FlutterDesktopMessengerRef messenger,
    const FlutterDesktopMessageResponseHandle* handle,
    const uint8_t* data,
    size_t data_length) {
  messenger->engine->SendPlatformMessageResponse(handle, data, data_length);
}

void FlutterDesktopMessengerSetCallback(FlutterDesktopMessengerRef messenger,
                                        const char* channel,
                                        FlutterDesktopMessageCallback callback,
                                        void* user_data) {
  messenger->engine->message_dispatcher()->SetMessageCallback(channel, callback,
                                                              user_data);
}

void FlutterDesktopEngineNotifyAppControl(FlutterDesktopEngineRef engine,
                                          void* app_control) {
  EngineFromHandle(engine)->app_control_channel()->NotifyAppControl(
      app_control);
}

void FlutterDesktopEngineNotifyLocaleChange(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->SetupLocales();
}

void FlutterDesktopEngineNotifyLowMemoryWarning(
    FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->NotifyLowMemoryWarning();
}

void FlutterDesktopEngineNotifyAppIsInactive(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsInactive();
}

void FlutterDesktopEngineNotifyAppIsResumed(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsResumed();
}

void FlutterDesktopEngineNotifyAppIsPaused(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsPaused();
}

void FlutterDesktopEngineNotifyAppIsDetached(FlutterDesktopEngineRef engine) {
  EngineFromHandle(engine)->lifecycle_channel()->AppIsDetached();
}

void FlutterDesktopViewDestroy(FlutterDesktopViewRef view_ref) {
  flutter::FlutterTizenView* view = ViewFromHandle(view_ref);
  delete view;
}

FlutterDesktopViewRef FlutterDesktopViewCreateFromNewWindow(
    const FlutterDesktopWindowProperties& window_properties,
    FlutterDesktopEngineRef engine) {
  flutter::TizenGeometry window_geometry = {
      window_properties.x, window_properties.y, window_properties.width,
      window_properties.height};

  std::unique_ptr<flutter::TizenWindow> window;
  if (window_properties.renderer_type == FlutterDesktopRendererType::kEvasGL) {
    window = std::make_unique<flutter::TizenWindowElementary>(
        window_geometry, window_properties.transparent,
        window_properties.focusable, window_properties.top_level);
  } else {
#ifndef WEARABLE_PROFILE
    window = std::make_unique<flutter::TizenWindowEcoreWl2>(
        window_geometry, window_properties.transparent,
        window_properties.focusable, window_properties.top_level);
#else
    return nullptr;
#endif
  }

  auto view = std::make_unique<flutter::FlutterTizenView>(std::move(window));

  // Take ownership of the engine, starting it if necessary.
  view->SetEngine(
      std::unique_ptr<flutter::FlutterTizenEngine>(EngineFromHandle(engine)));
  view->CreateRenderSurface(window_properties.renderer_type);
  if (!view->engine()->IsRunning()) {
    if (!view->engine()->RunEngine()) {
      return nullptr;
    }
  }

  view->SendInitialGeometry();

  return HandleForView(view.release());
}

void* FlutterDesktopViewGetNativeHandle(FlutterDesktopViewRef view_ref) {
  flutter::FlutterTizenView* view = ViewFromHandle(view_ref);
  auto* tizen_view =
      reinterpret_cast<flutter::TizenViewBase*>(view->tizen_view());
  return tizen_view->GetNativeHandle();
}

void FlutterDesktopViewResize(FlutterDesktopViewRef view,
                              int32_t width,
                              int32_t height) {
  ViewFromHandle(view)->Resize(width, height);
}

void FlutterDesktopViewOnPointerEvent(FlutterDesktopViewRef view,
                                      FlutterDesktopPointerEventType type,
                                      double x,
                                      double y,
                                      size_t timestamp,
                                      int32_t device_id) {
  switch (type) {
    case FlutterDesktopPointerEventType::kMouseDown:
    default:
      ViewFromHandle(view)->OnPointerDown(
          x, y, timestamp, kFlutterPointerDeviceKindTouch, device_id);
      break;
    case FlutterDesktopPointerEventType::kMouseUp:
      ViewFromHandle(view)->OnPointerUp(
          x, y, timestamp, kFlutterPointerDeviceKindTouch, device_id);
      break;
    case FlutterDesktopPointerEventType::kMouseMove:
      ViewFromHandle(view)->OnPointerMove(
          x, y, timestamp, kFlutterPointerDeviceKindTouch, device_id);
      break;
  }
}

void FlutterDesktopViewOnKeyEvent(FlutterDesktopViewRef view,
                                  const char* device_name,
                                  uint32_t device_class,
                                  uint32_t device_subclass,
                                  const char* key,
                                  const char* string,
                                  uint32_t modifiers,
                                  uint32_t scan_code,
                                  size_t timestamp,
                                  bool is_down) {
#ifdef NUI_SUPPORT
  auto* tizen_view = reinterpret_cast<flutter::TizenViewBase*>(
      ViewFromHandle(view)->tizen_view());

  if (tizen_view->GetType() == flutter::TizenViewType::kView &&
      ViewFromHandle(view)->engine()->renderer()->type() ==
          FlutterDesktopRendererType::kEGL) {
    reinterpret_cast<flutter::TizenViewNui*>(tizen_view)
        ->OnKey(device_name, device_class, device_subclass, key, string,
                nullptr, modifiers, scan_code, timestamp, is_down);
  }
#else
  ViewFromHandle(view)->OnKey(key, string, nullptr, modifiers, scan_code,
                              is_down);
#endif
}

void FlutterDesktopViewSetFocus(FlutterDesktopViewRef view, bool focused) {
  auto* tizen_view = reinterpret_cast<flutter::TizenViewBase*>(
      ViewFromHandle(view)->tizen_view());
  if (tizen_view->GetType() == flutter::TizenViewType::kView) {
    reinterpret_cast<flutter::TizenView*>(tizen_view)->SetFocus(focused);
  }
}

bool FlutterDesktopViewIsFocused(FlutterDesktopViewRef view) {
  auto* tizen_view = reinterpret_cast<flutter::TizenViewBase*>(
      ViewFromHandle(view)->tizen_view());
  if (tizen_view->GetType() == flutter::TizenViewType::kView) {
    return reinterpret_cast<flutter::TizenView*>(tizen_view)->focused();
  }
  return false;
}

void FlutterDesktopRegisterViewFactory(
    FlutterDesktopPluginRegistrarRef registrar,
    const char* view_type,
    std::unique_ptr<PlatformViewFactory> view_factory) {
  registrar->engine->platform_view_channel()->ViewFactories().insert(
      std::pair<std::string, std::unique_ptr<PlatformViewFactory>>(
          view_type, std::move(view_factory)));
}

FlutterDesktopTextureRegistrarRef FlutterDesktopRegistrarGetTextureRegistrar(
    FlutterDesktopPluginRegistrarRef registrar) {
  return HandleForTextureRegistrar(registrar->engine->texture_registrar());
}

int64_t FlutterDesktopTextureRegistrarRegisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    const FlutterDesktopTextureInfo* texture_info) {
  return TextureRegistrarFromHandle(texture_registrar)
      ->RegisterTexture(texture_info);
}

bool FlutterDesktopTextureRegistrarUnregisterExternalTexture(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    int64_t texture_id) {
  return TextureRegistrarFromHandle(texture_registrar)
      ->UnregisterTexture(texture_id);
}

bool FlutterDesktopTextureRegistrarMarkExternalTextureFrameAvailable(
    FlutterDesktopTextureRegistrarRef texture_registrar,
    int64_t texture_id) {
  return TextureRegistrarFromHandle(texture_registrar)
      ->MarkTextureFrameAvailable(texture_id);
}
