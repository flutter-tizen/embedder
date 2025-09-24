// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/external_texture_surface_vulkan.h"
#include "flutter/shell/platform/tizen/external_texture_surface_vulkan_buffer_dma.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

ExternalTextureSurfaceVulkan::ExternalTextureSurfaceVulkan(
    FlutterDesktopGpuSurfaceTextureCallback texture_callback,
    void* user_data,
    TizenRendererVulkan* vulkan_renderer)
    : ExternalVulkanTexture(),
      texture_callback_(texture_callback),
      user_data_(user_data),
      vulkan_renderer_(vulkan_renderer) {}

ExternalTextureSurfaceVulkan::~ExternalTextureSurfaceVulkan() {}

bool ExternalTextureSurfaceVulkan::CreateOrUpdateImage(
    const FlutterDesktopGpuSurfaceDescriptor* descriptor) {
  if (descriptor == nullptr || descriptor->handle == nullptr) {
    if (vulkan_buffer_) {
      vulkan_buffer_->ReleaseImage();
    }
    return false;
  }
  const tbm_surface_h tbm_surface =
      reinterpret_cast<tbm_surface_h>(descriptor->handle);
  if (!vulkan_buffer_) {
    if (IsSupportDisjoint(tbm_surface)) {
      /**
      vulkan_buffer_ = std::make_unique<ExternalTextureSurfaceVulkanBufferMap>(
          vulkan_renderer_);
      */
    } else {
      vulkan_buffer_ = std::make_unique<ExternalTextureSurfaceVulkanBufferDma>(
          vulkan_renderer_);
    }
  }
  void* handle = descriptor->handle;
  if (handle != last_surface_handle_) {
    vulkan_buffer_->ReleaseImage();
    tbm_surface_info_s tbm_surface_info;
    if (tbm_surface_get_info(tbm_surface, &tbm_surface_info) !=
        TBM_SURFACE_ERROR_NONE) {
      if (descriptor->release_callback) {
        descriptor->release_callback(descriptor->release_context);
      }
      return false;
    }
    if (!vulkan_buffer_->CreateImage(tbm_surface)) {
      FT_LOG(Error) << "Fail to create image";
      vulkan_buffer_->ReleaseImage();
      return false;
    }
    if (!vulkan_buffer_->AllocateMemory(tbm_surface)) {
      FT_LOG(Error) << "Fail to allocate memory";
      vulkan_buffer_->ReleaseImage();
      return false;
    }
    if (!vulkan_buffer_->BindImageMemory(tbm_surface)) {
      FT_LOG(Error) << "Fail to bind image memory";
      vulkan_buffer_->ReleaseImage();
      return false;
    }
    last_surface_handle_ = handle;
  }
  if (descriptor->release_callback) {
    descriptor->release_callback(descriptor->release_context);
  }
  return true;
}

bool ExternalTextureSurfaceVulkan::IsSupportDisjoint(
    tbm_surface_h tbm_surface) {
  int num_bos = tbm_surface_internal_get_num_bos(tbm_surface);
  bool is_disjoint = false;
  uint32_t tfd[num_bos];
  for (int i = 0; i < num_bos; i++) {
    tbm_bo bo = tbm_surface_internal_get_bo(tbm_surface, i);
    tfd[i] = tbm_bo_get_handle(bo, TBM_DEVICE_3D).u32;
    if (tfd[i] != tfd[0])
      is_disjoint = true;
  }
  return is_disjoint;
}

bool ExternalTextureSurfaceVulkan::PopulateVulkanTexture(
    size_t width,
    size_t height,
    FlutterVulkanTexture* vulkan_texture) {
  if (!texture_callback_) {
    return false;
  }
  const FlutterDesktopGpuSurfaceDescriptor* gpu_surface =
      texture_callback_(width, height, user_data_);
  if (!gpu_surface) {
    FT_LOG(Info) << "gpu_surface is null for texture ID: " << texture_id_;
    return false;
  }

  if (!CreateOrUpdateImage(gpu_surface)) {
    FT_LOG(Info) << "CreateOrUpdateEglImage fail for texture ID: "
                 << texture_id_;
    return false;
  }

  vulkan_texture->image =
      reinterpret_cast<uint64_t>(vulkan_buffer_->GetImage());
  vulkan_texture->format = vulkan_buffer_->GetFormat();
  vulkan_texture->image_memory =
      reinterpret_cast<uint64_t>(vulkan_buffer_->GetMemory());
  vulkan_texture->width = width;
  vulkan_texture->height = height;
  return true;
}

}  // namespace flutter
