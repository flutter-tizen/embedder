// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_H_

#include <vulkan/vulkan.h>
#include <memory>
#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/external_texture_surface_vulkan_buffer.h"
#include "flutter/shell/platform/tizen/tizen_renderer_vulkan.h"

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <tbm_type.h>

namespace flutter {
class ExternalTextureSurfaceVulkan : public ExternalVulkanTexture {
 public:
  ExternalTextureSurfaceVulkan(
      FlutterDesktopGpuSurfaceTextureCallback texture_callback,
      void* user_data,
      TizenRendererVulkan* vulkan_renderer);

  virtual ~ExternalTextureSurfaceVulkan();

  bool PopulateVulkanTexture(size_t width,
                             size_t height,
                             FlutterVulkanTexture* vulkan_texture) override;

 private:
  bool CreateOrUpdateImage(
      const FlutterDesktopGpuSurfaceDescriptor* descriptor);
  bool IsSupportDisjoint(tbm_surface_h tbm_surface);
  FlutterDesktopGpuSurfaceTextureCallback texture_callback_ = nullptr;
  void* user_data_ = nullptr;
  TizenRendererVulkan* vulkan_renderer_ = nullptr;
  void* last_surface_handle_ = nullptr;
  std::unique_ptr<ExternalTextureSurfaceVulkanBuffer> vulkan_buffer_;
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_H_
