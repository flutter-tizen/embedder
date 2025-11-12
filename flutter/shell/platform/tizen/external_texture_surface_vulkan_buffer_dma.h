// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_BUFFER_DMA_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_BUFFER_DMA_H_

#include <vulkan/vulkan.h>
#include "flutter/shell/platform/tizen/external_texture_surface_vulkan_buffer.h"
#include "flutter/shell/platform/tizen/tizen_renderer_vulkan.h"

#include <tbm_surface.h>

namespace flutter {
class ExternalTextureSurfaceVulkanBufferDma
    : public ExternalTextureSurfaceVulkanBuffer {
 public:
  ExternalTextureSurfaceVulkanBufferDma(TizenRendererVulkan* vulkan_renderer);
  virtual ~ExternalTextureSurfaceVulkanBufferDma();
  bool CreateImage(tbm_surface_h tbm_surface) override;
  void ReleaseImage() override;
  bool AllocateMemory(tbm_surface_h tbm_surface) override;
  bool BindImageMemory(tbm_surface_h tbm_surface) override;
  VkFormat GetFormat() override;
  VkImage GetImage() override;
  VkDeviceMemory GetMemory() override;

 private:
  bool GetFdMemoryTypeIndex(int fd, uint32_t& index_out);
  VkFormat texture_format_ = VK_FORMAT_UNDEFINED;
  VkImage texture_image_ = VK_NULL_HANDLE;
  VkDeviceMemory texture_device_memory_ = VK_NULL_HANDLE;
  PFN_vkGetMemoryFdPropertiesKHR getMemoryFdPropertiesKHR_ = nullptr;
  VkDevice device_;
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_SURFACE_VULKAN_BUFFER_DMA_H_
