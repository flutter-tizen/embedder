// Copyright 2025 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EMBEDDER_TIZEN_RENDERER_VULKAN_H_
#define EMBEDDER_TIZEN_RENDERER_VULKAN_H_

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) || defined(__unix__)
#define VK_USE_PLATFORM_XLIB_KHR
#elif defined(__APPLE__)
#define VK_USE_PLATFORM_MACOS_MVK
#else
#endif
#define VK_NO_PROTOTYPES
#include "flutter/shell/platform/tizen/tizen_renderer.h"
#include "flutter/shell/platform/tizen/tizen_view_base.h"
#include "flutter/shell/platform/tizen/volk.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>

namespace flutter {

class TizenRendererVulkan : public TizenRenderer {
 public:
  explicit TizenRendererVulkan(TizenViewBase* view);
  virtual ~TizenRendererVulkan();

  std::unique_ptr<ExternalTexture> CreateExternalTexture(
      const FlutterDesktopTextureInfo* texture_info) override;

  FlutterRendererConfig GetRendererConfig() override;

  bool CreateSurface(void* render_target,
                     void* render_target_display,
                     int32_t width,
                     int32_t height) override;
  void DestroySurface() override;
  void ResizeSurface(int32_t width, int32_t height) override;
  uint32_t GetVersion();
  FlutterVulkanInstanceHandle GetInstanceHandle();
  FlutterVulkanQueueHandle GetQueueHandle();
  FlutterVulkanPhysicalDeviceHandle GetPhysicalDeviceHandle();
  FlutterVulkanDeviceHandle GetDeviceHandle();
  uint32_t GetQueueIndex();
  size_t GetEnabledInstanceExtensionCount();
  const char** GetEnabledInstanceExtensions();
  size_t GetEnabledDeviceExtensionCount();
  const char** GetEnabledDeviceExtensions();
  void* GetInstanceProcAddress(FlutterVulkanInstanceHandle instance,
                               const char* name);
  FlutterVulkanImage GetNextImage(const FlutterFrameInfo* frameInfo);
  bool Present(const FlutterVulkanImage* image);
  VkCommandBuffer BeginSingleTimeCommands();
  void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

 private:
  bool CreateCommandPool();
  bool CreateInstance();
  bool CreateLogicalDevice();
  bool CreateFence();
  bool CreateSemaphore();
  void Cleanup();
  bool CheckValidationLayerSupport();
  void DestroyCommandPool();
  bool GetDeviceQueue();
  bool GetRequiredExtensions(std::vector<const char*>& extensions);
  VkSurfaceFormatKHR GetSwapChainFormat(
      std::vector<VkSurfaceFormatKHR>& surface_formats);
  VkExtent2D GetSwapChainExtent(VkSurfaceCapabilitiesKHR& surface_capabilities);
  uint32_t GetSwapChainNumImages(
      VkSurfaceCapabilitiesKHR& surface_capabilities);
  VkPresentModeKHR GetSwapChainPresentMode(
      std::vector<VkPresentModeKHR>& present_modes);
  VkCompositeAlphaFlagBitsKHR GetSwapChainCompositeAlpha(
      VkSurfaceCapabilitiesKHR& surface_capabilities);
  bool InitVulkan(TizenViewBase* view);
  bool InitializeSwapchain();
  bool RecreateSwapChain();
  void PopulateDebugMessengerCreateInfo(
      VkDebugUtilsMessengerCreateInfoEXT& createInfo);
  bool PickPhysicalDevice();
  void SetupDebugMessenger();

  bool enable_validation_layers_ = false;

  VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
  VkDevice logical_device_ = VK_NULL_HANDLE;
  VkInstance instance_ = VK_NULL_HANDLE;
  VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
  VkQueue graphics_queue_ = VK_NULL_HANDLE;
  VkSurfaceKHR surface_ = VK_NULL_HANDLE;
  VkSurfaceFormatKHR surface_format_;
  VkSemaphore present_transition_semaphore_ = VK_NULL_HANDLE;
  VkFence image_ready_fence_ = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
  VkCommandPool swapchain_command_pool_ = VK_NULL_HANDLE;
  std::vector<VkImage> swapchain_images_;
  std::vector<VkCommandBuffer> present_transition_buffers_;
  std::vector<const char*> enabled_device_extensions_;
  std::vector<const char*> enabled_instance_extensions_;
  uint32_t graphics_queue_family_index_ = 0;
  uint32_t last_image_index_ = 0;
  bool resize_pending_ = false;
  int32_t width_ = 0;
  int32_t height_ = 0;
};
}  // namespace flutter

#endif  // EMBEDDER_TIZEN_RENDERER_VULKAN_H_
