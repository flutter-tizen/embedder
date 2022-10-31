// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"

#include <Ecore.h>

#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/tizen/testing/engine_modifier.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

class FlutterTizenEngineTest : public ::testing::Test {
 public:
  FlutterTizenEngineTest() { ecore_init(); }

 protected:
  void SetUp() {
    FlutterDesktopEngineProperties engine_prop = {};
    engine_prop.assets_path = "/foo/flutter_assets";
    engine_prop.icu_data_path = "/foo/icudtl.dat";
    engine_prop.aot_library_path = "/foo/libapp.so";

    FlutterProjectBundle project(engine_prop);
    auto engine = std::make_unique<FlutterTizenEngine>(project);

    EngineModifier modifier(engine.get());
    // Force the non-AOT path unless overridden by the test.
    modifier.embedder_api().RunsAOTCompiledDartCode = []() { return false; };

    engine_ = engine.release();
  }

  void TearDown() {
    if (engine_) {
      delete engine_;
    }
    engine_ = nullptr;
  }

  FlutterTizenEngine* engine_ = nullptr;
};

TEST_F(FlutterTizenEngineTest, RunDoesExpectedInitialization) {
  EngineModifier modifier(engine_);
  bool run_called = false;
  modifier.embedder_api().Run = MOCK_ENGINE_PROC(
      Run, ([&run_called, engine_instance = engine_](
                size_t version, const FlutterRendererConfig* config,
                const FlutterProjectArgs* args, void* user_data,
                FLUTTER_API_SYMBOL(FlutterEngine) * engine_out) {
        run_called = true;
        *engine_out = reinterpret_cast<FLUTTER_API_SYMBOL(FlutterEngine)>(1);

        EXPECT_EQ(version, (size_t)FLUTTER_ENGINE_VERSION);
        EXPECT_NE(config, nullptr);
        EXPECT_EQ(user_data, engine_instance);

        EXPECT_STREQ(args->assets_path, "/foo/flutter_assets");
        EXPECT_STREQ(args->icu_data_path, "/foo/icudtl.dat");
        EXPECT_EQ(args->dart_entrypoint_argc, 0);
        EXPECT_NE(args->platform_message_callback, nullptr);
        EXPECT_NE(args->custom_task_runners, nullptr);
        EXPECT_EQ(args->custom_dart_entrypoint, nullptr);

        return kSuccess;
      }));

  // It should send locale info.
  bool update_locales_called = false;
  modifier.embedder_api().UpdateLocales = MOCK_ENGINE_PROC(
      UpdateLocales,
      ([&update_locales_called](auto engine, const FlutterLocale** locales,
                                size_t locales_count) {
        update_locales_called = true;
        EXPECT_GT(locales_count, (size_t)0);
        EXPECT_NE(locales, nullptr);

        return kSuccess;
      }));

  // And it should send initial settings info.
  bool settings_message_sent = false;
  modifier.embedder_api().SendPlatformMessage = MOCK_ENGINE_PROC(
      SendPlatformMessage,
      ([&settings_message_sent](auto engine, auto message) {
        if (std::string(message->channel) == std::string("flutter/settings")) {
          settings_message_sent = true;
        }

        return kSuccess;
      }));

  engine_->RunEngine();

  EXPECT_TRUE(run_called);
  EXPECT_TRUE(update_locales_called);
  EXPECT_TRUE(settings_message_sent);
  EXPECT_NE(engine_->plugin_registrar(), nullptr);
  EXPECT_EQ(engine_->texture_registrar(), nullptr);

  modifier.embedder_api().Shutdown = [](auto engine) { return kSuccess; };
}

TEST_F(FlutterTizenEngineTest, SendPlatformMessageWithoutResponse) {
  EngineModifier modifier(engine_);

  const char* channel = "test";
  const std::vector<uint8_t> test_message = {1, 2, 3, 4};

  // Without a response, SendPlatformMessage should be a simple pass-through.
  bool called = false;
  modifier.embedder_api().SendPlatformMessage = MOCK_ENGINE_PROC(
      SendPlatformMessage, ([&called, test_message](auto engine, auto message) {
        called = true;
        EXPECT_STREQ(message->channel, "test");
        EXPECT_EQ(message->message_size, test_message.size());
        EXPECT_EQ(memcmp(message->message, test_message.data(),
                         message->message_size),
                  0);
        EXPECT_EQ(message->response_handle, nullptr);
        return kSuccess;
      }));

  engine_->SendPlatformMessage(channel, test_message.data(),
                               test_message.size(), nullptr, nullptr);
  EXPECT_TRUE(called);
}

TEST_F(FlutterTizenEngineTest, SendPlatformMessageWithResponse) {
  EngineModifier modifier(engine_);

  const char* channel = "test";
  const std::vector<uint8_t> test_message = {1, 2, 3, 4};
  auto* dummy_response_handle =
      reinterpret_cast<FlutterPlatformMessageResponseHandle*>(5);
  const FlutterDesktopBinaryReply reply_handler = [](auto... args) {};
  void* reply_user_data = reinterpret_cast<void*>(6);

  // When a response is requested, a handle should be created, passed as part
  // of the message, and then released.
  bool create_response_handle_called = false;
  modifier.embedder_api().PlatformMessageCreateResponseHandle =
      MOCK_ENGINE_PROC(
          PlatformMessageCreateResponseHandle,
          ([&create_response_handle_called, &reply_handler, reply_user_data,
            dummy_response_handle](auto engine, auto reply, auto user_data,
                                   auto response_handle) {
            create_response_handle_called = true;
            EXPECT_EQ(reply, reply_handler);
            EXPECT_EQ(user_data, reply_user_data);
            EXPECT_NE(response_handle, nullptr);
            *response_handle = dummy_response_handle;
            return kSuccess;
          }));
  bool release_response_handle_called = false;
  modifier.embedder_api().PlatformMessageReleaseResponseHandle =
      MOCK_ENGINE_PROC(
          PlatformMessageReleaseResponseHandle,
          ([&release_response_handle_called, dummy_response_handle](
               auto engine, auto response_handle) {
            release_response_handle_called = true;
            EXPECT_EQ(response_handle, dummy_response_handle);
            return kSuccess;
          }));
  bool send_message_called = false;
  modifier.embedder_api().SendPlatformMessage = MOCK_ENGINE_PROC(
      SendPlatformMessage, ([&send_message_called, test_message,
                             dummy_response_handle](auto engine, auto message) {
        send_message_called = true;
        EXPECT_STREQ(message->channel, "test");
        EXPECT_EQ(message->message_size, test_message.size());
        EXPECT_EQ(memcmp(message->message, test_message.data(),
                         message->message_size),
                  0);
        EXPECT_EQ(message->response_handle, dummy_response_handle);
        return kSuccess;
      }));

  engine_->SendPlatformMessage(channel, test_message.data(),
                               test_message.size(), reply_handler,
                               reply_user_data);
  EXPECT_TRUE(create_response_handle_called);
  EXPECT_TRUE(release_response_handle_called);
  EXPECT_TRUE(send_message_called);
}

TEST_F(FlutterTizenEngineTest, AddPluginRegistrarDestructionCallback) {
  EngineModifier modifier(engine_);
  modifier.embedder_api().Run = MOCK_ENGINE_PROC(
      Run, ([](size_t version, const FlutterRendererConfig* config,
               const FlutterProjectArgs* args, void* user_data,
               FLUTTER_API_SYMBOL(FlutterEngine) * engine_out) {
        *engine_out = reinterpret_cast<FLUTTER_API_SYMBOL(FlutterEngine)>(1);
        return kSuccess;
      }));

  // Stub out UpdateLocales and SendPlatformMessage as we don't have a fully
  // initialized engine instance.
  modifier.embedder_api().UpdateLocales = MOCK_ENGINE_PROC(
      UpdateLocales, ([](auto engine, const FlutterLocale** locales,
                         size_t locales_count) { return kSuccess; }));
  modifier.embedder_api().SendPlatformMessage =
      MOCK_ENGINE_PROC(SendPlatformMessage,
                       ([](auto engine, auto message) { return kSuccess; }));

  engine_->RunEngine();

  // Verify that destruction handlers don't overwrite each other.
  int result1 = 0;
  int result2 = 0;
  engine_->AddPluginRegistrarDestructionCallback(
      [](FlutterDesktopPluginRegistrarRef ref) {
        auto result = reinterpret_cast<int*>(ref);
        *result = 1;
      },
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(&result1));
  engine_->AddPluginRegistrarDestructionCallback(
      [](FlutterDesktopPluginRegistrarRef ref) {
        auto result = reinterpret_cast<int*>(ref);
        *result = 2;
      },
      reinterpret_cast<FlutterDesktopPluginRegistrarRef>(&result2));

  // Ensure that deallocation doesn't call the actual Shutdown with the bogus
  // engine pointer that the overridden Run returned.
  modifier.embedder_api().Shutdown = [](auto engine) { return kSuccess; };

  engine_->StopEngine();
  EXPECT_EQ(result1, 1);
  EXPECT_EQ(result2, 2);
}

}  // namespace testing
}  // namespace flutter
