// Copyright 2020 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_FLUTTER_TIZEN_TEXTURE_REGISTRAR_H_
#define EMBEDDER_FLUTTER_TIZEN_TEXTURE_REGISTRAR_H_

#include <memory>
#include <mutex>
#include <unordered_map>

#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/public/flutter_tizen.h"

namespace flutter {

class FlutterTizenEngine;

// An object managing the registration of an external texture.
// Thread safety: All member methods are thread safe.
class FlutterTizenTextureRegistrar {
 public:
  explicit FlutterTizenTextureRegistrar(FlutterTizenEngine* engine, bool enable_impeller);

  // Registers a texture described by the given |texture_info| object.
  //
  // Returns a non-zero positive texture id, or -1 on error.
  int64_t RegisterTexture(const FlutterDesktopTextureInfo* texture_info);

  // Attempts to unregister the texture identified by |texture_id|.
  //
  // Returns true if the texture was successfully unregistered.
  bool UnregisterTexture(int64_t texture_id);

  // Notifies the engine about a new frame being available.
  //
  // Returns true on success.
  bool MarkTextureFrameAvailable(int64_t texture_id);

  // Attempts to populate the given |texture| by copying the
  // contents of the texture identified by |texture_id|.
  //
  // Returns true on success.
  bool PopulateTexture(int64_t texture_id,
                       size_t width,
                       size_t height,
                       FlutterOpenGLTexture* texture);

  std::unique_ptr<ExternalTexture> CreateExternalTexture(
      const FlutterDesktopTextureInfo* info,
      FlutterDesktopRendererType renderer_type);

 private:
  FlutterTizenEngine* engine_ = nullptr;

  // All registered textures, keyed by their IDs.
  std::unordered_map<int64_t, std::unique_ptr<ExternalTexture>> textures_;
  std::mutex map_mutex_;
  bool enable_impeller_ = false;
};

}  // namespace flutter

#endif  // EMBEDDER_FLUTTER_TIZEN_TEXTURE_REGISTRAR_H_
