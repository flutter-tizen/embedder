// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/flutter_tizen_texture_registrar.h"

#include <Ecore.h>

#include <iostream>

#include "flutter/shell/platform/embedder/test_utils/proc_table_replacement.h"
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/testing/engine_modifier.h"
#include "gtest/gtest.h"

namespace flutter {
namespace testing {

class FlutterTizenTextureRegistrarTest : public ::testing::Test {
 public:
  FlutterTizenTextureRegistrarTest() { ecore_init(); }

 protected:
  void SetUp() {
    FlutterDesktopEngineProperties engine_prop = {};
    engine_prop.assets_path = "/foo/flutter_assets";
    engine_prop.icu_data_path = "/foo/icudtl.dat";
    engine_prop.aot_library_path = "/foo/libapp.so";

    FlutterProjectBundle project(engine_prop);
    auto engine = std::make_unique<FlutterTizenEngine>(project);
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

TEST_F(FlutterTizenTextureRegistrarTest, CreateDestroy) {
  FlutterTizenTextureRegistrar registrar(engine_,false);

  EXPECT_TRUE(true);
}

TEST_F(FlutterTizenTextureRegistrarTest, RegisterUnregisterTexture) {
  EngineModifier modifier(engine_);

  FlutterTizenTextureRegistrar registrar(engine_,false);

  FlutterDesktopTextureInfo texture_info = {};
  texture_info.type = kFlutterDesktopGpuSurfaceTexture;
  texture_info.gpu_surface_config.callback =
      [](size_t width, size_t height,
         void* user_data) -> const FlutterDesktopGpuSurfaceDescriptor* {
    return nullptr;
  };

  int64_t registered_texture_id = 0;
  bool register_called = false;
  modifier.embedder_api().RegisterExternalTexture = MOCK_ENGINE_PROC(
      RegisterExternalTexture, ([&register_called, &registered_texture_id](
                                    auto engine, auto texture_id) {
        register_called = true;
        registered_texture_id = texture_id;
        return kSuccess;
      }));

  bool unregister_called = false;
  modifier.embedder_api().UnregisterExternalTexture = MOCK_ENGINE_PROC(
      UnregisterExternalTexture, ([&unregister_called, &registered_texture_id](
                                      auto engine, auto texture_id) {
        unregister_called = true;
        EXPECT_EQ(registered_texture_id, texture_id);
        return kSuccess;
      }));

  bool mark_frame_available_called = false;
  modifier.embedder_api().MarkExternalTextureFrameAvailable =
      MOCK_ENGINE_PROC(MarkExternalTextureFrameAvailable,
                       ([&mark_frame_available_called, &registered_texture_id](
                            auto engine, auto texture_id) {
                         mark_frame_available_called = true;
                         EXPECT_EQ(registered_texture_id, texture_id);
                         return kSuccess;
                       }));

  int64_t texture_id = registrar.RegisterTexture(&texture_info);
  EXPECT_TRUE(register_called);
  EXPECT_NE(texture_id, -1);
  EXPECT_EQ(texture_id, registered_texture_id);

  EXPECT_TRUE(registrar.MarkTextureFrameAvailable(texture_id));
  EXPECT_TRUE(mark_frame_available_called);

  EXPECT_TRUE(registrar.UnregisterTexture(texture_id));
  EXPECT_TRUE(unregister_called);
}

TEST_F(FlutterTizenTextureRegistrarTest, RegisterUnknownTextureType) {
  EngineModifier modifier(engine_);

  FlutterTizenTextureRegistrar registrar(engine_,false);

  FlutterDesktopTextureInfo texture_info = {};
  texture_info.type = static_cast<FlutterDesktopTextureType>(1234);

  int64_t texture_id = registrar.RegisterTexture(&texture_info);

  EXPECT_EQ(texture_id, -1);
}

TEST_F(FlutterTizenTextureRegistrarTest, PopulateInvalidTexture) {
  FlutterTizenTextureRegistrar registrar(engine_,false);

  bool result = registrar.PopulateTexture(1, 640, 480, nullptr);
  EXPECT_FALSE(result);
}

}  // namespace testing
}  // namespace flutter
