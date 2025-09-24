// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/external_texture_pixel_vulkan.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {
ExternalTexturePixelVulkan::ExternalTexturePixelVulkan(
    FlutterDesktopPixelBufferTextureCallback texture_callback,
    void* user_data,
    TizenRendererVulkan* vulkan_renderer)
    : ExternalVulkanTexture(),
      texture_callback_(texture_callback),
      user_data_(user_data),
      vulkan_renderer_(vulkan_renderer) {}

ExternalTexturePixelVulkan::~ExternalTexturePixelVulkan() {
  ReleaseImage();
}

bool ExternalTexturePixelVulkan::PopulateVulkanTexture(
    size_t width,
    size_t height,
    FlutterVulkanTexture* flutter_texture) {
  if (!texture_callback_) {
    FT_LOG(Error) << "texture_callback_ is nullptr";
    return false;
  }

  const FlutterDesktopPixelBuffer* pixel_buffer =
      texture_callback_(width, height, user_data_);

  if (!pixel_buffer) {
    FT_LOG(Error) << "pixel_buffer is nullptr";
    return false;
  }

  if (!CreateOrUpdateImage(pixel_buffer)) {
    FT_LOG(Error) << "Fail to create image";
    ReleaseImage();
    return false;
  }
  width_ = pixel_buffer->width;
  height_ = pixel_buffer->height;

  if (!CreateBuffer(width, height)) {
    return false;
  }

  CopyBufferToImage(staging_buffer_, vk_image_, width_, height_,
                    pixel_buffer->buffer);

  ReleaseBuffer();
  FlutterVulkanTexture* vulkan_texture =
      static_cast<FlutterVulkanTexture*>(flutter_texture);
  vulkan_texture->image = reinterpret_cast<uint64_t>(vk_image_);
  vulkan_texture->format = VK_FORMAT_R8G8B8A8_UNORM;
  vulkan_texture->width = width;
  vulkan_texture->height = height;
  return true;
}

bool ExternalTexturePixelVulkan::CreateOrUpdateImage(
    const FlutterDesktopPixelBuffer* pixel_buffer) {
  if (vk_image_ == VK_NULL_HANDLE) {
    return CreateImage(pixel_buffer->width, pixel_buffer->height);
  }

  if (pixel_buffer->width != width_ || pixel_buffer->height != height_) {
    ReleaseImage();
    return CreateImage(pixel_buffer->width, pixel_buffer->height);
  }
  return true;
}

bool ExternalTexturePixelVulkan::CreateImage(size_t width, size_t height) {
  VkImageCreateInfo image_info{};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
  image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (vkCreateImage(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                    &image_info, nullptr, &vk_image_) != VK_SUCCESS) {
    FT_LOG(Error) << "Fail to create VkImage";
    return false;
  }
  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(
      static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()), vk_image_,
      &memory_requirements);
  uint32_t memory_type_index;
  if (!FindMemoryType(memory_requirements.memoryTypeBits,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, memory_type_index)) {
    return false;
  }
  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = memory_requirements.size;
  alloc_info.memoryTypeIndex = memory_type_index;

  if (vkAllocateMemory(
          static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
          &alloc_info, nullptr, &vk_image_memory_) != VK_SUCCESS) {
    FT_LOG(Error) << "Fail to allocate memory";
    return false;
  }

  if (vkBindImageMemory(
          static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()), vk_image_,
          vk_image_memory_, 0) != VK_SUCCESS) {
    FT_LOG(Error) << "Fail to bind image memory";
    return false;
  }
  return true;
}

bool ExternalTexturePixelVulkan::FindMemoryType(
    uint32_t type_filter,
    VkMemoryPropertyFlags properties,
    uint32_t& index_out) {
  VkPhysicalDeviceMemoryProperties memory_properties;
  vkGetPhysicalDeviceMemoryProperties(
      static_cast<VkPhysicalDevice>(
          vulkan_renderer_->GetPhysicalDeviceHandle()),
      &memory_properties);

  for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
    if ((type_filter & (1 << i)) &&
        (memory_properties.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      index_out = i;
      return true;
    }
  }
  return false;
}

bool ExternalTexturePixelVulkan::CreateBuffer(size_t width, size_t height) {
  VkBufferCreateInfo buffer_info{};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = width * height * 4;
  buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                     &buffer_info, nullptr, &staging_buffer_) != VK_SUCCESS) {
    FT_LOG(Error) << "Fail to create vkBuffer";
    return false;
  }

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(
      static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
      staging_buffer_, &memory_requirements);
  uint32_t memory_type_index;
  if (!FindMemoryType(memory_requirements.memoryTypeBits,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      memory_type_index)) {
    FT_LOG(Error) << "Fail to find memory type";
    return false;
  }
  VkMemoryAllocateInfo alloc_info{};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = memory_requirements.size;
  alloc_info.memoryTypeIndex = memory_type_index;

  if (vkAllocateMemory(
          static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
          &alloc_info, nullptr, &staging_buffer_memory_) != VK_SUCCESS) {
    FT_LOG(Error) << "Fail to allocate memory";
    return false;
  }

  if (vkBindBufferMemory(
          static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
          staging_buffer_, staging_buffer_memory_, 0) != VK_SUCCESS) {
    FT_LOG(Error) << "Fail to bind buffer memory";
    return false;
  }
  return true;
}

void ExternalTexturePixelVulkan::ReleaseBuffer() {
  if (staging_buffer_ != VK_NULL_HANDLE) {
    vkDestroyBuffer(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                    staging_buffer_, nullptr);
    staging_buffer_ = VK_NULL_HANDLE;
  }
  if (staging_buffer_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                 staging_buffer_memory_, nullptr);

    staging_buffer_memory_ = VK_NULL_HANDLE;
  }
}

void ExternalTexturePixelVulkan::CopyBufferToImage(VkBuffer dst_buffer,
                                                   VkImage image,
                                                   uint32_t width,
                                                   uint32_t height,
                                                   const uint8_t* src_buffer) {
  void* data;
  vkMapMemory(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
              staging_buffer_memory_, 0, width * height * 4, 0, &data);
  memcpy(data, src_buffer, static_cast<size_t>(width * height * 4));
  vkUnmapMemory(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                staging_buffer_memory_);
  VkCommandBuffer command_buffer = vulkan_renderer_->BeginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(command_buffer, dst_buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  vulkan_renderer_->EndSingleTimeCommands(command_buffer);
}

void ExternalTexturePixelVulkan::ReleaseImage() {
  if (vk_image_ != VK_NULL_HANDLE) {
    vkDestroyImage(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                   vk_image_, nullptr);
    vk_image_ = VK_NULL_HANDLE;
  }

  if (vk_image_memory_ != VK_NULL_HANDLE) {
    vkFreeMemory(static_cast<VkDevice>(vulkan_renderer_->GetDeviceHandle()),
                 vk_image_memory_, nullptr);
    vk_image_memory_ = VK_NULL_HANDLE;
  }
}

}  // namespace flutter
