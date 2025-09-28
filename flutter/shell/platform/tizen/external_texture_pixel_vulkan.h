// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_PIXEL_VULKAN_H_
#define FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_PIXEL_VULKAN_H_

#include "flutter/shell/platform/common/public/flutter_texture_registrar.h"
#include "flutter/shell/platform/embedder/embedder.h"
#include "flutter/shell/platform/tizen/external_texture.h"
#include "flutter/shell/platform/tizen/tizen_renderer_vulkan.h"

namespace flutter {
class ExternalTexturePixelVulkan : public ExternalVulkanTexture {
 public:
  ExternalTexturePixelVulkan(
      FlutterDesktopPixelBufferTextureCallback texture_callback,
      void* user_data,
      TizenRendererVulkan* vulkan_renderer);

  virtual ~ExternalTexturePixelVulkan();

  bool PopulateVulkanTexture(size_t width,
                             size_t height,
                             FlutterVulkanTexture* flutter_texture) override;

 private:
  bool AllocateMemory(const VkMemoryRequirements& memory_requirements, VkDeviceMemory& memory, VkMemoryPropertyFlags properties); 
  bool CreateBuffer(VkDeviceSize required_size);
  bool CreateImage(size_t width, size_t height);
  bool CreateOrUpdateBuffer(VkDeviceSize required_size);
  bool CreateOrUpdateImage(size_t width, size_t height);
  void CopyBufferToImage(const uint8_t* src_buffer, VkDeviceSize size);
  void ReleaseBuffer();
  void ReleaseImage();
  bool FindMemoryType(uint32_t typeFilter,
                      VkMemoryPropertyFlags properties,
                      uint32_t& index_out);
  FlutterDesktopPixelBufferTextureCallback texture_callback_ = nullptr;
  size_t width_ = 0;
  size_t height_ = 0;
  void* user_data_ = nullptr;
  TizenRendererVulkan* vulkan_renderer_ = nullptr;
  VkImage vk_image_ = VK_NULL_HANDLE;
  VkDeviceMemory vk_image_memory_ = VK_NULL_HANDLE;
  VkBuffer staging_buffer_ = VK_NULL_HANDLE;
  VkDeviceMemory staging_buffer_memory_ = VK_NULL_HANDLE;
  VkDeviceSize staging_buffer_size_ = 0;
};
}  // namespace flutter

#endif  // FLUTTER_SHELL_PLATFORM_TIZEN_EXTERNAL_TEXTURE_PIXEL_VULKAN_H_
