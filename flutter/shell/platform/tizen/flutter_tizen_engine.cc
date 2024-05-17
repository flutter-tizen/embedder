// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_tizen_engine.h"

#include <algorithm>
#include <string>
#include <vector>

#include "flutter/shell/platform/tizen/accessibility_bridge_tizen.h"
#include "flutter/shell/platform/tizen/flutter_platform_node_delegate_tizen.h"
#include "flutter/shell/platform/tizen/flutter_tizen_view.h"
#include "flutter/shell/platform/tizen/logger.h"
#include "flutter/shell/platform/tizen/system_utils.h"
#include "flutter/shell/platform/tizen/tizen_input_method_context.h"
#include "flutter/shell/platform/tizen/tizen_renderer_egl.h"
#include "flutter/shell/platform/tizen/tizen_renderer_evas_gl.h"

namespace flutter {

namespace {

// Unique number associated with platform tasks.
constexpr size_t kPlatformTaskRunnerIdentifier = 1;
constexpr size_t kRenderTaskRunnerIdentifier = 2;

// Converts a LanguageInfo struct to a FlutterLocale struct. |info| must outlive
// the returned value, since the returned FlutterLocale has pointers into it.
FlutterLocale CovertToFlutterLocale(const LanguageInfo& info) {
  FlutterLocale locale = {};
  locale.struct_size = sizeof(FlutterLocale);
  locale.language_code = info.language.c_str();
  if (!info.country.empty()) {
    locale.country_code = info.country.c_str();
  }
  if (!info.script.empty()) {
    locale.script_code = info.script.c_str();
  }
  if (!info.variant.empty()) {
    locale.variant_code = info.variant.c_str();
  }
  return locale;
}

}  // namespace

FlutterTizenEngine::FlutterTizenEngine(const FlutterProjectBundle& project)
    : project_(std::make_unique<FlutterProjectBundle>(project)),
      aot_data_(nullptr, nullptr) {
  embedder_api_.struct_size = sizeof(FlutterEngineProcTable);
  FlutterEngineGetProcAddresses(&embedder_api_);

  // Run flutter task on Tizen main loop.
  // Tizen engine has four threads (GPU thread, UI thread, IO thread, platform
  // thread). UI threads need to send flutter task to platform thread.
  event_loop_ = std::make_unique<TizenPlatformEventLoop>(
      std::this_thread::get_id(),  // main thread
      embedder_api_.GetCurrentTime, [this](const auto* task) {
        if (embedder_api_.RunTask(this->engine_, task) != kSuccess) {
          FT_LOG(Error) << "Could not post an engine task.";
        }
      });

  messenger_ = std::make_unique<FlutterDesktopMessenger>();
  messenger_->engine = this;
  message_dispatcher_ =
      std::make_unique<IncomingMessageDispatcher>(messenger_.get());

  plugin_registrar_ = std::make_unique<FlutterDesktopPluginRegistrar>();
  plugin_registrar_->engine = this;
}

FlutterTizenEngine::~FlutterTizenEngine() {
  StopEngine();
}

void FlutterTizenEngine::CreateRenderer(
    FlutterDesktopRendererType renderer_type) {
  if (renderer_type == FlutterDesktopRendererType::kEvasGL) {
    renderer_ = std::make_unique<TizenRendererEvasGL>();

    render_loop_ = std::make_unique<TizenRenderEventLoop>(
        std::this_thread::get_id(),  // main thread
        embedder_api_.GetCurrentTime,
        [this](const auto* task) {
          if (embedder_api_.RunTask(this->engine_, task) != kSuccess) {
            FT_LOG(Error) << "Could not post an engine task.";
          }
        },
        renderer_.get());
  } else {
    renderer_ = std::make_unique<TizenRendererEgl>();
  }
}

bool FlutterTizenEngine::RunEngine() {
  if (engine_ != nullptr) {
    FT_LOG(Error) << "The engine has already started.";
    return false;
  }
  if (IsHeaded() && !renderer_->IsValid()) {
    FT_LOG(Error) << "The display was not valid.";
    return false;
  }

  if (!project_->HasValidPaths()) {
    FT_LOG(Error) << "Missing or unresolvable paths to assets.";
    return false;
  }
  std::string assets_path_string = project_->assets_path().u8string();
  std::string icu_path_string = project_->icu_path().u8string();
  if (embedder_api_.RunsAOTCompiledDartCode()) {
    aot_data_ = project_->LoadAotData(embedder_api_);
    if (!aot_data_) {
      FT_LOG(Error) << "Unable to start engine without AOT data.";
      return false;
    }
  }

  // FlutterProjectArgs is expecting a full argv, so when processing it for
  // flags the first item is treated as the executable and ignored. Add a dummy
  // value so that all provided arguments are used.
  std::vector<std::string> engine_args = project_->engine_arguments();
  std::vector<const char*> engine_argv = {"placeholder"};
  std::transform(
      engine_args.begin(), engine_args.end(), std::back_inserter(engine_argv),
      [](const std::string& arg) -> const char* { return arg.c_str(); });

  const std::vector<std::string>& entrypoint_args =
      project_->dart_entrypoint_arguments();
  std::vector<const char*> entrypoint_argv;
  std::transform(
      entrypoint_args.begin(), entrypoint_args.end(),
      std::back_inserter(entrypoint_argv),
      [](const std::string& arg) -> const char* { return arg.c_str(); });

  // Configure task runners.
  FlutterTaskRunnerDescription platform_task_runner = {};
  platform_task_runner.struct_size = sizeof(FlutterTaskRunnerDescription);
  platform_task_runner.user_data = event_loop_.get();
  platform_task_runner.runs_task_on_current_thread_callback =
      [](void* data) -> bool {
    return static_cast<TizenEventLoop*>(data)->RunsTasksOnCurrentThread();
  };
  platform_task_runner.post_task_callback =
      [](FlutterTask task, uint64_t target_time_nanos, void* data) -> void {
    static_cast<TizenEventLoop*>(data)->PostTask(task, target_time_nanos);
  };
  platform_task_runner.identifier = kPlatformTaskRunnerIdentifier;
  FlutterCustomTaskRunners custom_task_runners = {};
  custom_task_runners.struct_size = sizeof(FlutterCustomTaskRunners);
  custom_task_runners.platform_task_runner = &platform_task_runner;

  FlutterTaskRunnerDescription render_task_runner = {};

  if (IsHeaded() && dynamic_cast<TizenRendererEvasGL*>(renderer_.get())) {
    render_task_runner.struct_size = sizeof(FlutterTaskRunnerDescription);
    render_task_runner.user_data = render_loop_.get();
    render_task_runner.runs_task_on_current_thread_callback =
        [](void* data) -> bool {
      return static_cast<TizenEventLoop*>(data)->RunsTasksOnCurrentThread();
    };
    render_task_runner.post_task_callback =
        [](FlutterTask task, uint64_t target_time_nanos, void* data) -> void {
      static_cast<TizenEventLoop*>(data)->PostTask(task, target_time_nanos);
    };
    render_task_runner.identifier = kRenderTaskRunnerIdentifier;
    custom_task_runners.render_task_runner = &render_task_runner;
  }

  FlutterProjectArgs args = {};
  args.struct_size = sizeof(FlutterProjectArgs);
  args.assets_path = assets_path_string.c_str();
  args.icu_data_path = icu_path_string.c_str();
  args.command_line_argc = static_cast<int>(engine_argv.size());
  args.command_line_argv =
      engine_argv.size() > 0 ? engine_argv.data() : nullptr;
  args.dart_entrypoint_argc = static_cast<int>(entrypoint_argv.size());
  args.dart_entrypoint_argv =
      entrypoint_argv.size() > 0 ? entrypoint_argv.data() : nullptr;
  args.platform_message_callback =
      [](const FlutterPlatformMessage* engine_message, void* user_data) {
        if (engine_message->struct_size != sizeof(FlutterPlatformMessage)) {
          FT_LOG(Error) << "Invalid message size received. Expected: "
                        << sizeof(FlutterPlatformMessage) << ", but received "
                        << engine_message->struct_size;
          return;
        }
        auto* engine = static_cast<FlutterTizenEngine*>(user_data);
        FlutterDesktopMessage message =
            engine->ConvertToDesktopMessage(*engine_message);
        engine->message_dispatcher_->HandleMessage(message);
      };
  args.custom_task_runners = &custom_task_runners;
  if (aot_data_) {
    args.aot_data = aot_data_.get();
  }
  if (!project_->custom_dart_entrypoint().empty()) {
    args.custom_dart_entrypoint = project_->custom_dart_entrypoint().c_str();
  }

  args.update_semantics_callback2 = [](const FlutterSemanticsUpdate2* update,
                                       void* user_data) {
    auto* engine = static_cast<FlutterTizenEngine*>(user_data);
    engine->OnUpdateSemantics(update);
  };

  if (IsHeaded() && dynamic_cast<TizenRendererEgl*>(renderer_.get())) {
    vsync_waiter_ = std::make_unique<TizenVsyncWaiter>(this);
    args.vsync_callback = [](void* user_data, intptr_t baton) -> void {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      std::lock_guard<std::mutex> lock(engine->vsync_mutex_);
      if (engine->vsync_waiter_) {
        engine->vsync_waiter_->AsyncWaitForVsync(baton);
      }
    };
  }

  FlutterRendererConfig renderer_config = GetRendererConfig();

  FlutterEngineResult result = embedder_api_.Run(
      FLUTTER_ENGINE_VERSION, &renderer_config, &args, this, &engine_);
  if (result != kSuccess || engine_ == nullptr) {
    FT_LOG(Error) << "Failed to start the Flutter engine with error: "
                  << result;
    return false;
  }

  internal_plugin_registrar_ =
      std::make_unique<PluginRegistrar>(plugin_registrar_.get());
  accessibility_channel_ = std::make_unique<AccessibilityChannel>(
      internal_plugin_registrar_->messenger());
  app_control_channel_ = std::make_unique<AppControlChannel>(
      internal_plugin_registrar_->messenger());
  lifecycle_channel_ = std::make_unique<LifecycleChannel>(
      internal_plugin_registrar_->messenger());
  settings_channel_ = std::make_unique<SettingsChannel>(
      internal_plugin_registrar_->messenger());

  if (IsHeaded()) {
    texture_registrar_ = std::make_unique<FlutterTizenTextureRegistrar>(this);
    keyboard_channel_ = std::make_unique<KeyboardChannel>(
        internal_plugin_registrar_->messenger(),
        [this](const FlutterKeyEvent& event, FlutterKeyEventCallback callback,
               void* user_data) { SendKeyEvent(event, callback, user_data); });
    navigation_channel_ = std::make_unique<NavigationChannel>(
        internal_plugin_registrar_->messenger());
  }

  accessibility_settings_ = std::make_unique<AccessibilitySettings>(this);

  SetupLocales();

  return true;
}

bool FlutterTizenEngine::StopEngine() {
  if (engine_) {
    for (const auto& [callback, registrar] :
         plugin_registrar_destruction_callbacks_) {
      callback(registrar);
    }

    {
      std::lock_guard<std::mutex> lock(vsync_mutex_);
      if (vsync_waiter_) {
        vsync_waiter_.reset();
      }
    }

    FlutterEngineResult result = embedder_api_.Shutdown(engine_);
    view_ = nullptr;
    engine_ = nullptr;
    return (result == kSuccess);
  }
  return false;
}

void FlutterTizenEngine::SetView(FlutterTizenView* view) {
  view_ = view;
}

void FlutterTizenEngine::AddPluginRegistrarDestructionCallback(
    FlutterDesktopOnPluginRegistrarDestroyed callback,
    FlutterDesktopPluginRegistrarRef registrar) {
  plugin_registrar_destruction_callbacks_[callback] = registrar;
}

bool FlutterTizenEngine::SendPlatformMessage(
    const char* channel,
    const uint8_t* message,
    const size_t message_size,
    const FlutterDesktopBinaryReply reply,
    void* user_data) {
  FlutterPlatformMessageResponseHandle* response_handle = nullptr;
  if (reply != nullptr) {
    FlutterEngineResult result =
        embedder_api_.PlatformMessageCreateResponseHandle(
            engine_, reply, user_data, &response_handle);
    if (result != kSuccess) {
      FT_LOG(Error) << "Failed to create a response handle.";
      return false;
    }
  }

  FlutterPlatformMessage platform_message = {
      sizeof(FlutterPlatformMessage),
      channel,
      message,
      message_size,
      response_handle,
  };

  FlutterEngineResult message_result =
      embedder_api_.SendPlatformMessage(engine_, &platform_message);
  if (response_handle != nullptr) {
    embedder_api_.PlatformMessageReleaseResponseHandle(engine_,
                                                       response_handle);
  }
  return message_result == kSuccess;
}

void FlutterTizenEngine::SendPlatformMessageResponse(
    const FlutterDesktopMessageResponseHandle* handle,
    const uint8_t* data,
    size_t data_length) {
  embedder_api_.SendPlatformMessageResponse(engine_, handle, data, data_length);
}

void FlutterTizenEngine::SendKeyEvent(const FlutterKeyEvent& event,
                                      FlutterKeyEventCallback callback,
                                      void* user_data) {
  embedder_api_.SendKeyEvent(engine_, &event, callback, user_data);
}

void FlutterTizenEngine::SendPointerEvent(const FlutterPointerEvent& event) {
  embedder_api_.SendPointerEvent(engine_, &event, 1);
}

void FlutterTizenEngine::SendWindowMetrics(int32_t x,
                                           int32_t y,
                                           int32_t width,
                                           int32_t height,
                                           double pixel_ratio) {
  FlutterWindowMetricsEvent event = {};
  event.struct_size = sizeof(FlutterWindowMetricsEvent);
  event.left = static_cast<size_t>(x);
  event.top = static_cast<size_t>(y);
  event.width = static_cast<size_t>(width);
  event.height = static_cast<size_t>(height);
  event.pixel_ratio = pixel_ratio;
  embedder_api_.SendWindowMetricsEvent(engine_, &event);
}

void FlutterTizenEngine::OnVsync(intptr_t baton,
                                 uint64_t frame_start_time_nanos,
                                 uint64_t frame_target_time_nanos) {
  embedder_api_.OnVsync(engine_, baton, frame_start_time_nanos,
                        frame_target_time_nanos);
}

void FlutterTizenEngine::SetupLocales() {
  std::vector<LanguageInfo> languages = GetPreferredLanguageInfo();
  std::vector<FlutterLocale> flutter_locales;
  flutter_locales.reserve(languages.size());
  for (const LanguageInfo& info : languages) {
    flutter_locales.push_back(CovertToFlutterLocale(info));
  }
  // Convert the locale list to the locale pointer list that must be provided.
  std::vector<const FlutterLocale*> flutter_locale_list;
  flutter_locale_list.reserve(flutter_locales.size());
  std::transform(flutter_locales.begin(), flutter_locales.end(),
                 std::back_inserter(flutter_locale_list),
                 [](const auto& arg) -> const auto* { return &arg; });

  embedder_api_.UpdateLocales(engine_, flutter_locale_list.data(),
                              flutter_locale_list.size());
}

void FlutterTizenEngine::NotifyLowMemoryWarning() {
  embedder_api_.NotifyLowMemoryWarning(engine_);
}

bool FlutterTizenEngine::RegisterExternalTexture(int64_t texture_id) {
  return (embedder_api_.RegisterExternalTexture(engine_, texture_id) ==
          kSuccess);
}

bool FlutterTizenEngine::UnregisterExternalTexture(int64_t texture_id) {
  return (embedder_api_.UnregisterExternalTexture(engine_, texture_id) ==
          kSuccess);
}

bool FlutterTizenEngine::MarkExternalTextureFrameAvailable(int64_t texture_id) {
  return (embedder_api_.MarkExternalTextureFrameAvailable(
              engine_, texture_id) == kSuccess);
}

void FlutterTizenEngine::UpdateAccessibilityFeatures(bool invert_colors,
                                                     bool high_contrast) {
  int32_t flags = 0;
  flags |= invert_colors ? kFlutterAccessibilityFeatureInvertColors : 0;
  flags |= high_contrast ? kFlutterAccessibilityFeatureHighContrast : 0;
  embedder_api_.UpdateAccessibilityFeatures(engine_,
                                            FlutterAccessibilityFeature(flags));
}

FlutterDesktopMessage FlutterTizenEngine::ConvertToDesktopMessage(
    const FlutterPlatformMessage& engine_message) {
  FlutterDesktopMessage message = {};
  message.struct_size = sizeof(message);
  message.channel = engine_message.channel;
  message.message = engine_message.message;
  message.message_size = engine_message.message_size;
  message.response_handle = engine_message.response_handle;
  return message;
}

FlutterRendererConfig FlutterTizenEngine::GetRendererConfig() {
  FlutterRendererConfig config = {};
  if (IsHeaded()) {
    config.type = kOpenGL;
    config.open_gl.struct_size = sizeof(config.open_gl);
    config.open_gl.make_current = [](void* user_data) -> bool {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return false;
      }
      return engine->view()->OnMakeCurrent();
    };
    config.open_gl.make_resource_current = [](void* user_data) -> bool {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return false;
      }
      return engine->view()->OnMakeResourceCurrent();
    };
    config.open_gl.clear_current = [](void* user_data) -> bool {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return false;
      }
      return engine->view()->OnClearCurrent();
    };
    config.open_gl.present = [](void* user_data) -> bool {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return false;
      }
      return engine->view()->OnPresent();
    };
    config.open_gl.fbo_callback = [](void* user_data) -> uint32_t {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return false;
      }
      return engine->view()->OnGetFBO();
    };
    config.open_gl.surface_transformation =
        [](void* user_data) -> FlutterTransformation {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return FlutterTransformation();
      }
      return engine->view()->GetFlutterTransformation();
    };
    config.open_gl.gl_proc_resolver = [](void* user_data,
                                         const char* name) -> void* {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->view()) {
        return nullptr;
      }
      return engine->view()->OnProcResolver(name);
    };
    config.open_gl.gl_external_texture_frame_callback =
        [](void* user_data, int64_t texture_id, size_t width, size_t height,
           FlutterOpenGLTexture* texture) -> bool {
      auto* engine = static_cast<FlutterTizenEngine*>(user_data);
      if (!engine->texture_registrar()) {
        return false;
      }
      return engine->texture_registrar()->PopulateTexture(texture_id, width,
                                                          height, texture);
    };
  } else {
    config.type = kSoftware;
    config.software.struct_size = sizeof(config.software);
    config.software.surface_present_callback =
        [](void* user_data, const void* allocation, size_t row_bytes,
           size_t height) -> bool { return true; };
  }
  return config;
}

void FlutterTizenEngine::DispatchAccessibilityAction(
    uint64_t target,
    FlutterSemanticsAction action,
    fml::MallocMapping data) {
  embedder_api_.DispatchSemanticsAction(engine_, target, action,
                                        data.GetMapping(), data.GetSize());
}

void FlutterTizenEngine::SetSemanticsEnabled(bool enabled) {
  FT_LOG(Debug) << "Accessibility enabled: " << enabled;
  if (!enabled && accessibility_bridge_) {
    accessibility_bridge_.reset();
  } else if (enabled && !accessibility_bridge_) {
    accessibility_bridge_ = std::make_shared<AccessibilityBridgeTizen>(this);
  }

  FlutterPlatformAppDelegateTizen::GetInstance().SetAccessibilityStatus(
      enabled);

  embedder_api_.UpdateSemanticsEnabled(engine_, enabled);
}

void FlutterTizenEngine::OnUpdateSemantics(
    const FlutterSemanticsUpdate2* update) {
  if (!accessibility_bridge_) {
    FT_LOG(Error) << "The accessibility bridge must be initialized.";
    return;
  }

  for (size_t i = 0; i < update->node_count; i++) {
    const FlutterSemanticsNode2* node = update->nodes[i];
    accessibility_bridge_->AddFlutterSemanticsNodeUpdate(*node);
  }

  for (size_t i = 0; i < update->custom_action_count; i++) {
    const FlutterSemanticsCustomAction2* action = update->custom_actions[i];
    accessibility_bridge_->AddFlutterSemanticsCustomActionUpdate(*action);
  }

  accessibility_bridge_->CommitUpdates();

  // Attaches the accessibility root to the window delegate.
  std::weak_ptr<FlutterPlatformNodeDelegate> root =
      accessibility_bridge_->GetFlutterPlatformNodeDelegateFromID(0);
  std::shared_ptr<FlutterPlatformWindowDelegateTizen> window =
      FlutterPlatformAppDelegateTizen::GetInstance().GetWindow().lock();
  TizenGeometry geometry = view_->tizen_view()->GetGeometry();
  window->SetGeometry(geometry.left, geometry.top, geometry.width,
                      geometry.height);
  window->SetRootNode(root);
}

}  // namespace flutter
