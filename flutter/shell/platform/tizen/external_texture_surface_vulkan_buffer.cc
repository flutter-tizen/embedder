// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/external_texture_surface_vulkan_buffer.h"
#include <vulkan/vulkan.h>
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

ExternalTextureSurfaceVulkanBuffer::ExternalTextureSurfaceVulkanBuffer(
    TizenRendererVulkan* vulkan_renderer)
    : vulkan_renderer_(vulkan_renderer) {}

VkFormat ExternalTextureSurfaceVulkanBuffer::ConvertFormat(tbm_format format) {
  switch (format) {
    case TBM_FORMAT_NV12:
    case TBM_FORMAT_NV21:
      return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    case TBM_FORMAT_RGBA8888:
    case TBM_FORMAT_ABGR8888:
    case TBM_FORMAT_RGBX8888:
    case TBM_FORMAT_XRGB8888:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case TBM_FORMAT_XBGR8888:
    case TBM_FORMAT_BGRX8888:
    case TBM_FORMAT_ARGB8888:
    case TBM_FORMAT_BGRA8888:
      return VK_FORMAT_B8G8R8A8_UNORM;
    default:
      FT_LOG(Warn) << "Unknown TBM format: " << format
                   << ", returning VK_FORMAT_UNDEFINED";
      return VK_FORMAT_UNDEFINED;
  }
}

VkDevice ExternalTextureSurfaceVulkanBuffer::GetDevice() const {
  return static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle());
}

bool ExternalTextureSurfaceVulkanBuffer::TransitionToShaderReadLayout(
    VkImage image) {
  if (image == VK_NULL_HANDLE) {
    FT_LOG(Error) << "Cannot transition layout of a null VkImage";
    return false;
  }

  VkCommandBuffer command_buffer = vulkan_renderer_->BeginSingleTimeCommands();
  if (command_buffer == VK_NULL_HANDLE) {
    FT_LOG(Error) << "Failed to begin single time commands";
    return false;
  }

  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
  vulkan_renderer_->EndSingleTimeCommands(command_buffer);
  return true;
}

}  // namespace flutter
