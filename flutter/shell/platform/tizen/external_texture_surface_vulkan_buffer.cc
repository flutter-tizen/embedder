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

VkFormat ExternalTextureSurfaceVulkanBuffer::ConvertFormat(
    const tbm_format& format) {
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

}  // namespace flutter
