// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_BUFFER_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_BUFFER_H_

#include <vulkan/vulkan.h>
#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/tizen_renderer_vulkan.h"

#include <tbm_bufmgr.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <tbm_type.h>

namespace flutter {
class ExternalTextureSurfaceVulkanBuffer {
 public:
  ExternalTextureSurfaceVulkanBuffer(TizenRendererVulkan* vulkan_renderer);

  virtual ~ExternalTextureSurfaceVulkanBuffer() = default;
  virtual bool CreateImage(tbm_surface_h tbm_surface) = 0;
  virtual void ReleaseImage() = 0;
  virtual bool AllocateMemory(tbm_surface_h tbm_surface) = 0;
  virtual bool BindImageMemory(tbm_surface_h tbm_surface) = 0;
  virtual VkFormat GetFormat() = 0;
  virtual VkImage GetImage() = 0;
  virtual VkDeviceMemory GetMemory() = 0;

 protected:
  VkFormat ConvertFormat(tbm_format& format);
  bool FindProperties(uint32_t memory_type_bits_requirement,
                      VkMemoryPropertyFlags required_properties,
                      uint32_t& index_out);

 private:
  TizenRendererVulkan* vulkan_renderer_ = nullptr;
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_H_
