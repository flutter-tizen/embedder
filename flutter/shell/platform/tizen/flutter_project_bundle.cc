// Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter_project_bundle.h"

#include <app_common.h>
#include <linux/limits.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>

#include "flutter/shell/platform/common/path_utils.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

namespace {

// Returns the path of the directory containing the app binary, or an empty
// string if the directory cannot be found.
std::filesystem::path GetBinDirectory() {
  char* res_path = app_get_resource_path();
  if (!res_path) {
    return GetExecutableDirectory();
  }
  auto bin_path = std::filesystem::path(res_path) / ".." / "bin";
  free(res_path);
  return bin_path.lexically_normal();
}

}  // namespace

FlutterProjectBundle::FlutterProjectBundle(
    const FlutterDesktopEngineProperties& properties)
    : assets_path_(properties.assets_path),
      icu_path_(properties.icu_data_path) {
  if (properties.aot_library_path != nullptr) {
    aot_library_path_ = std::filesystem::path(properties.aot_library_path);
  }

  if (properties.entrypoint != nullptr) {
    custom_dart_entrypoint_ = std::string(properties.entrypoint);
  }

  for (int i = 0; i < properties.dart_entrypoint_argc; i++) {
    dart_entrypoint_arguments_.push_back(
        std::string(properties.dart_entrypoint_argv[i]));
  }

  merged_platform_ui_thread_ = properties.merged_platform_ui_thread;

  // Resolve any relative paths.
  if (assets_path_.is_relative() || icu_path_.is_relative() ||
      (!aot_library_path_.empty() && aot_library_path_.is_relative())) {
    std::filesystem::path executable_location = GetBinDirectory();
    if (executable_location.empty()) {
      FT_LOG(Error)
          << "Unable to find executable location to resolve resource paths.";
      assets_path_ = std::filesystem::path();
      icu_path_ = std::filesystem::path();
    } else {
      assets_path_ = std::filesystem::path(executable_location) / assets_path_;
      icu_path_ = std::filesystem::path(executable_location) / icu_path_;
      if (!aot_library_path_.empty()) {
        aot_library_path_ =
            std::filesystem::path(executable_location) / aot_library_path_;
      }
    }
  }

  engine_arguments_.insert(engine_arguments_.end(), properties.switches,
                           properties.switches + properties.switches_count);
}

bool FlutterProjectBundle::HasValidPaths() {
  return !assets_path_.empty() && !icu_path_.empty();
}

bool FlutterProjectBundle::HasArgument(const std::string& name) {
  return std::find(engine_arguments_.begin(), engine_arguments_.end(), name) !=
         engine_arguments_.end();
}

bool FlutterProjectBundle::GetArgumentValue(const std::string& name,
                                            std::string* value) {
  auto iter =
      std::find(engine_arguments_.begin(), engine_arguments_.end(), name);
  if (iter == engine_arguments_.end()) {
    return false;
  }
  auto next = std::next(iter);
  if (next == engine_arguments_.end()) {
    return false;
  }
  *value = *next;
  return true;
}

UniqueAotDataPtr FlutterProjectBundle::LoadAotData(
    const FlutterEngineProcTable& engine_procs) {
  if (aot_library_path_.empty()) {
    FT_LOG(Error)
        << "Attempted to load AOT data, but no aot_library_path was provided.";
    return UniqueAotDataPtr(nullptr, nullptr);
  }
  if (!std::filesystem::exists(aot_library_path_)) {
    FT_LOG(Error) << "Can't load AOT data from " << aot_library_path_.u8string()
                  << "; no such file.";
    return UniqueAotDataPtr(nullptr, nullptr);
  }
  std::string path_string = aot_library_path_.u8string();
  FlutterEngineAOTDataSource source = {};
  source.type = kFlutterEngineAOTDataSourceTypeElfPath;
  source.elf_path = path_string.c_str();
  FlutterEngineAOTData data = nullptr;
  FlutterEngineResult result = engine_procs.CreateAOTData(&source, &data);
  if (result != kSuccess) {
    FT_LOG(Error) << "Failed to load AOT data from: " << path_string;
    return UniqueAotDataPtr(nullptr, nullptr);
  }
  return UniqueAotDataPtr(data, engine_procs.CollectAOTData);
}

FlutterProjectBundle::~FlutterProjectBundle() {}

}  // namespace flutter
