// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/external_texture_surface_vulkan_buffer.h"
#include <vulkan/vulkan.h>

namespace flutter {

ExternalTextureSurfaceVulkanBuffer::ExternalTextureSurfaceVulkanBuffer(
    TizenRendererVulkan* vulkan_renderer)
    : vulkan_renderer_(vulkan_renderer) {}

VkFormat ExternalTextureSurfaceVulkanBuffer::ConvertFormat(tbm_format& format) {
  switch (format) {
    case TBM_FORMAT_NV12:
    case TBM_FORMAT_NV21:
      return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    case TBM_FORMAT_RGBA8888:
    case TBM_FORMAT_ARGB8888:
    case TBM_FORMAT_RGBX8888:
    case TBM_FORMAT_XRGB8888:
    case TBM_FORMAT_RGB888:
      return VK_FORMAT_R8G8B8A8_UNORM;
    case TBM_FORMAT_BGR888:
    case TBM_FORMAT_XBGR8888:
    case TBM_FORMAT_BGRX8888:
    case TBM_FORMAT_ABGR8888:
    case TBM_FORMAT_BGRA8888:
      return VK_FORMAT_B8G8R8A8_UNORM;
    default:
      return VK_FORMAT_UNDEFINED;
  }
}

bool ExternalTextureSurfaceVulkanBuffer::FindProperties(
    uint32_t memory_type_bits_requirement,
    VkMemoryPropertyFlags required_properties,
    uint32_t& index_out) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(
      static_cast<VkPhysicalDevice>(
          vulkan_renderer_->GetPhysicalDeviceHandle()),
      &memory_properties);

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    const uint32_t memory_type_bits = (1 << i);
    const bool isRequiredMemoryType =
        memory_type_bits_requirement & memory_type_bits;
    const VkMemoryPropertyFlags properties =
        memory_properties.memoryTypes[i].propertyFlags;
    const bool has_required_properties =
        (properties & required_properties) == required_properties;
    if (isRequiredMemoryType && has_required_properties) {
      index_out = i;
      return true;
    }
  }
  return false;
}

}  // namespace flutter
