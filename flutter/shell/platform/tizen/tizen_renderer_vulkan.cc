
// Copyright 2024 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "flutter/shell/platform/tizen/tizen_renderer_vulkan.h"
#include <stddef.h>
#include <vulkan/vulkan_wayland.h>
#include <optional>
#include "flutter/shell/platform/tizen/flutter_tizen_engine.h"
#include "flutter/shell/platform/tizen/logger.h"

namespace flutter {

const std::vector<const char*> validation_layers = {
    "VK_LAYER_KHRONOS_validation"};

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks* pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

TizenRendererVulkan::TizenRendererVulkan(TizenViewBase* view) {
  InitVulkan(view);
}

bool TizenRendererVulkan::InitVulkan(TizenViewBase* view) {
  if (!CreateInstance()) {
    FT_LOG(Error) << "Failed to create Vulkan instance";
    return false;
  }
  if (enable_validation_layers_) {
    SetupDebugMessenger();
  }

  if (!TizenRenderer::CreateSurface(view)) {
    FT_LOG(Error) << "Failed to create surface";
    return false;
  }
  if (!PickPhysicalDevice()) {
    FT_LOG(Error) << "Failed to pick physical device";
    return false;
  }
  if (!CreateLogicalDevice()) {
    FT_LOG(Error) << "Failed to create logical device";
    return false;
  }
  if (!GetDeviceQueue()) {
    FT_LOG(Error) << "Failed to get device queue";
    return false;
  }
  if (!CreateSemaphore()) {
    FT_LOG(Error) << "Failed to create semaphore";
    return false;
  }
  if (!CreateFence()) {
    FT_LOG(Error) << "Failed to create fence";
    return false;
  }
  if (!CreateCommandPool()) {
    FT_LOG(Error) << "Failed to create command pool";
    return false;
  }
  if (!InitializeSwapchain()) {
    FT_LOG(Error) << "Failed to initialize swapchain";
    return false;
  }
  is_valid_ = true;
  return true;
}

void TizenRendererVulkan::Cleanup() {
  if (logical_device_) {
    // Ensure all GPU work is complete before destroying anything.
    vkDeviceWaitIdle(logical_device_);

    for (size_t i = 0; i < present_transition_buffers_.size(); ++i) {
      vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                           &present_transition_buffers_[i]);
    }
    if (swapchain_ != VK_NULL_HANDLE) {
      vkDestroySwapchainKHR(logical_device_, swapchain_, nullptr);
      swapchain_ = VK_NULL_HANDLE;
    }
    DestroyCommandPool();
    vkDestroyFence(logical_device_, image_ready_fence_, nullptr);
    vkDestroySemaphore(logical_device_, present_transition_semaphore_, nullptr);
    vkDestroyDevice(logical_device_, nullptr);
    logical_device_ = VK_NULL_HANDLE;
  }
  DestroySurface();
  if (enable_validation_layers_) {
    DestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
  }
  if (instance_ != VK_NULL_HANDLE) {
    vkDestroyInstance(instance_, nullptr);
    instance_ = VK_NULL_HANDLE;
  }
}

std::unique_ptr<ExternalTexture> TizenRendererVulkan::CreateExternalTexture(
    const FlutterDesktopTextureInfo* texture_info) {
  return nullptr;
}

FlutterRendererConfig TizenRendererVulkan::GetRendererConfig() {
  FlutterRendererConfig config = {};
  config.type = kVulkan;
  config.vulkan.struct_size = sizeof(config.vulkan);
  config.vulkan.version = GetVersion();
  config.vulkan.instance = GetInstanceHandle();
  config.vulkan.physical_device = GetPhysicalDeviceHandle();
  config.vulkan.device = GetDeviceHandle();
  config.vulkan.queue = GetQueueHandle();
  config.vulkan.queue_family_index = GetQueueIndex();
  config.vulkan.enabled_instance_extension_count =
      GetEnabledInstanceExtensionCount();
  config.vulkan.enabled_instance_extensions = GetEnabledInstanceExtensions();
  config.vulkan.enabled_device_extension_count =
      GetEnabledDeviceExtensionCount();
  config.vulkan.enabled_device_extensions = GetEnabledDeviceExtensions();
  config.vulkan.get_instance_proc_address_callback =
      [](void* user_data, FlutterVulkanInstanceHandle instance,
         const char* name) -> void* {
    auto* engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return nullptr;
    }
    return dynamic_cast<TizenRendererVulkan*>(engine->renderer())
        ->GetInstanceProcAddress(instance, name);
  };
  config.vulkan.get_next_image_callback =
      [](void* user_data, const FlutterFrameInfo* frame) -> FlutterVulkanImage {
    auto* engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return FlutterVulkanImage();
    }
    return dynamic_cast<TizenRendererVulkan*>(engine->renderer())
        ->GetNextImage(frame);
  };
  config.vulkan.present_image_callback =
      [](void* user_data, const FlutterVulkanImage* image) -> bool {
    auto* engine = reinterpret_cast<FlutterTizenEngine*>(user_data);
    if (!engine->view()) {
      return false;
    }

    return dynamic_cast<TizenRendererVulkan*>(engine->renderer())
        ->Present(image);
  };
  return config;
}

bool TizenRendererVulkan::CreateInstance() {
  if (enable_validation_layers_ && !CheckValidationLayerSupport()) {
    FT_LOG(Error) << "Validation layers requested, but not available";
    return false;
  }

  if (!GetRequiredExtensions(enabled_instance_extensions_)) {
    FT_LOG(Error) << "Failed to get required extensions";
    return false;
  }

  // Create instance.
  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Tizen Embedder",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Tizen Embedder",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_1,
  };
  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount =
      static_cast<uint32_t>(enabled_instance_extensions_.size());
  create_info.ppEnabledExtensionNames = enabled_instance_extensions_.data();
  create_info.flags = 0;

  VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};

  if (enable_validation_layers_) {
    create_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
    PopulateDebugMessengerCreateInfo(debug_create_info);
    create_info.pNext = reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(
        &debug_create_info);
  } else {
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
  }

  if (vkCreateInstance(&create_info, nullptr, &instance_) != VK_SUCCESS) {
    FT_LOG(Error) << "Create instance failed.";
    return false;
  }
  return true;
}

bool TizenRendererVulkan::GetRequiredExtensions(
    std::vector<const char*>& extensions) {
  uint32_t instance_extension_count = 0;
  bool has_surface_extension = false;

  if (vkEnumerateInstanceExtensionProperties(nullptr, &instance_extension_count,
                                             nullptr) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to enumerate instance extension count";
    return false;
  }
  if (instance_extension_count > 0) {
    std::vector<VkExtensionProperties> instance_extension_properties(
        instance_extension_count);

    if (vkEnumerateInstanceExtensionProperties(
            nullptr, &instance_extension_count,
            instance_extension_properties.data()) != VK_SUCCESS) {
      FT_LOG(Error) << "Failed to enumerate instance extension properties";
      return false;
    }
    for (const auto& ext : instance_extension_properties) {
      if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, ext.extensionName)) {
        has_surface_extension = true;
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      } else if (!strcmp(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
                         ext.extensionName)) {
        has_surface_extension = true;
        extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
      } else if (!strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
                         ext.extensionName)) {
        if (enable_validation_layers_) {
          extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
      }
    }
  }

  if (!has_surface_extension) {
    FT_LOG(Error) << "vkEnumerateInstanceExtensionProperties failed to find "
                     "the extensions";
    return false;
  }

  return true;
}

void TizenRendererVulkan::SetupDebugMessenger() {
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  PopulateDebugMessengerCreateInfo(debug_create_info);

  if (CreateDebugUtilsMessengerEXT(instance_, &debug_create_info, nullptr,
                                   &debug_messenger_) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to set up debug messenger";
  }
}

bool TizenRendererVulkan::CheckValidationLayerSupport() {
  uint32_t layer_count;
  if (vkEnumerateInstanceLayerProperties(&layer_count, nullptr) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to enumerate instance layer properties";
    return false;
  }
  std::vector<VkLayerProperties> available_layers(layer_count);
  if (vkEnumerateInstanceLayerProperties(
          &layer_count, available_layers.data()) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to enumerate instance layer properties";
    return false;
  }

  for (const char* layer_name : validation_layers) {
    bool layer_found = false;
    for (const auto& layer_properties : available_layers) {
      FT_LOG(Info) << "layer_properties.layerName : "
                   << layer_properties.layerName;
      if (strcmp(layer_name, layer_properties.layerName) == 0) {
        layer_found = true;
        break;
      }
    }
    if (!layer_found) {
      FT_LOG(Error) << "Layer requested is not available";
      return false;
    }
  }
  return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  FT_LOG(Error) << pCallbackData->pMessage;
  return VK_FALSE;
}

void TizenRendererVulkan::PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = DebugMessengerCallback;
}

bool TizenRendererVulkan::PickPhysicalDevice() {
  uint32_t gpu_count = 0;
  if (vkEnumeratePhysicalDevices(instance_, &gpu_count, nullptr) !=
          VK_SUCCESS ||
      gpu_count == 0) {
    FT_LOG(Error) << "Error occurred during physical devices enumeration.";
    return false;
  }

  std::vector<VkPhysicalDevice> physical_devices(gpu_count);
  if (vkEnumeratePhysicalDevices(instance_, &gpu_count,
                                 physical_devices.data()) != VK_SUCCESS) {
    FT_LOG(Error) << "Error occurred during physical devices enumeration.";
    return false;
  }

  FT_LOG(Info) << "Enumerating " << gpu_count << " physical device(s).";

  uint32_t selected_score = 0;
  for (const auto& physical_device : physical_devices) {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    vkGetPhysicalDeviceFeatures(physical_device, &features);
    FT_LOG(Info) << "Checking device : " << properties.deviceName;
    uint32_t score = 0;
    std::vector<const char*> supported_extensions;
    uint32_t qfp_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &qfp_count,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> qfp(qfp_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &qfp_count,
                                             qfp.data());
    std::optional<uint32_t> graphics_queue_family;
    for (uint32_t i = 0; i < qfp.size(); i++) {
      // Only pick graphics queues that can also present to the surface.
      // Graphics queues that can't present are rare if not nonexistent, but
      // the spec allows for this, so check it anyhow.
      VkBool32 surface_present_supported;
      vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface_,
                                           &surface_present_supported);

      if (!graphics_queue_family.has_value() &&
          qfp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
          surface_present_supported) {
        graphics_queue_family = i;
      }
    }
    // Skip physical devices that don't have a graphics queue.
    if (!graphics_queue_family.has_value()) {
      FT_LOG(Info) << "Skipping due to no suitable graphics queues.";
      continue;
    }

    // Prefer discrete GPUs.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1 << 30;
    }
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                         &extension_count, nullptr);
    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(physical_device, nullptr,
                                         &extension_count,
                                         available_extensions.data());

    bool supports_swapchain = false;
    for (const auto& available_extension : available_extensions) {
      if (strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                 available_extension.extensionName) == 0) {
        supports_swapchain = true;
        supported_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
      } else if (strcmp("VK_KHR_portability_subset",
                        available_extension.extensionName) == 0) {
        supported_extensions.push_back("VK_KHR_portability_subset");
      } else if (strcmp(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
                        available_extension.extensionName) == 0) {
        score += 1 << 29;
        supported_extensions.push_back(
            VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
      } else if (strcmp(available_extension.extensionName,
                        VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME) == 0) {
        supported_extensions.push_back(
            VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME);
      } else if (strcmp(available_extension.extensionName,
                        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME) == 0) {
        supported_extensions.push_back(
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
      }
    }
    // Skip physical devices that don't have swapchain support.
    if (!supports_swapchain) {
      FT_LOG(Info) << "Skipping due to lack of swapchain support.";
      continue;
    }
    // Prefer GPUs with larger max texture sizes.
    score += properties.limits.maxImageDimension2D;
    if (selected_score < score) {
      FT_LOG(Info) << "This is the best device so far. Score: " << score;

      selected_score = score;
      physical_device_ = physical_device;
      enabled_device_extensions_ = supported_extensions;
      graphics_queue_family_index_ =
          graphics_queue_family.value_or(std::numeric_limits<uint32_t>::max());
    }
  }
  return physical_device_ != VK_NULL_HANDLE;
}

TizenRendererVulkan::~TizenRendererVulkan() {
  Cleanup();
}
bool TizenRendererVulkan::CreateSurface(void* render_target,
                                        void* render_target_display,
                                        int32_t width,
                                        int32_t height) {
  width_ = width;
  height_ = height;
  VkWaylandSurfaceCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
  createInfo.pNext = nullptr;
  createInfo.flags = 0;
  createInfo.display = static_cast<wl_display*>(render_target_display);
  createInfo.surface = static_cast<wl_surface*>(render_target);

  PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
  vkCreateWaylandSurfaceKHR =
      (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(
          instance_, "vkCreateWaylandSurfaceKHR");

  if (!vkCreateWaylandSurfaceKHR) {
    FT_LOG(Error) << "Wayland: Vulkan instance missing "
                     "VK_KHR_wayland_surface extension";
    return false;
  }

  if (vkCreateWaylandSurfaceKHR(instance_, &createInfo, nullptr, &surface_) !=
      VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create surface.";
    return false;
  }
  return true;
}

bool TizenRendererVulkan::CreateLogicalDevice() {
  VkDeviceQueueCreateInfo queue_info{};
  queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_info.queueFamilyIndex = graphics_queue_family_index_;
  queue_info.queueCount = 1;
  float priority = 1.0f;
  queue_info.pQueuePriorities = &priority;

  VkPhysicalDeviceFeatures device_features{};
  VkDeviceCreateInfo device_info{};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_info.queueCreateInfoCount = 1;
  device_info.pQueueCreateInfos = &queue_info;
  device_info.enabledExtensionCount =
      static_cast<uint32_t>(enabled_device_extensions_.size());
  device_info.ppEnabledExtensionNames = enabled_device_extensions_.data();
  device_info.pEnabledFeatures = &device_features;

  if (vkCreateDevice(physical_device_, &device_info, nullptr,
                     &logical_device_) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create device.";
    return false;
  }
  return true;
}

bool TizenRendererVulkan::GetDeviceQueue() {
  vkGetDeviceQueue(logical_device_, graphics_queue_family_index_, 0,
                   &graphics_queue_);
  return graphics_queue_ != VK_NULL_HANDLE;
}

bool TizenRendererVulkan::CreateCommandPool() {
  VkCommandPoolCreateInfo pool_info;
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.pNext = nullptr;
  pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                    VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  pool_info.queueFamilyIndex = graphics_queue_family_index_;
  if (vkCreateCommandPool(logical_device_, &pool_info, nullptr,
                          &swapchain_command_pool_) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create command pool.";
    return false;
  }
  return true;
}

void TizenRendererVulkan::DestroyCommandPool() {
  if (swapchain_command_pool_) {
    vkDestroyCommandPool(logical_device_, swapchain_command_pool_, nullptr);
    swapchain_command_pool_ = VK_NULL_HANDLE;
  }
}

bool TizenRendererVulkan::CreateSemaphore() {
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  if (vkCreateSemaphore(logical_device_, &semaphore_info, nullptr,
                        &present_transition_semaphore_) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create semaphore.";
    return false;
  }
  return true;
}

bool TizenRendererVulkan::CreateFence() {
  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(logical_device_, &fence_info, nullptr,
                    &image_ready_fence_) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to create image ready fence.";
    return false;
  }

  return true;
}

VkSurfaceFormatKHR TizenRendererVulkan::GetSwapChainFormat(
    std::vector<VkSurfaceFormatKHR>& surface_formats) {
  // If the list contains only one entry with undefined format
  // it means that there are no preferred surface formats and any can be chosen
  if ((surface_formats.size() == 1) &&
      (surface_formats[0].format == VK_FORMAT_UNDEFINED)) {
    return {VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
  }

  // Check if list contains most widely used R8 G8 B8 A8 format
  // with nonlinear color space
  for (VkSurfaceFormatKHR& surface_format : surface_formats) {
    if (surface_format.format == VK_FORMAT_R8G8B8A8_UNORM) {
      return surface_format;
    }
  }

  // Return the first format from the list
  return surface_formats[0];
}

VkExtent2D TizenRendererVulkan::GetSwapChainExtent(
    VkSurfaceCapabilitiesKHR& surface_capabilities) {
  if (surface_capabilities.currentExtent.width != UINT32_MAX) {
    // If the surface reports a specific extent, we must use it.
    return surface_capabilities.currentExtent;
  } else {
    VkExtent2D swap_chain_extent{};
    swap_chain_extent.width = width_;
    swap_chain_extent.height = height_;

    swap_chain_extent.width =
        std::max(surface_capabilities.minImageExtent.width,
                 std::min(surface_capabilities.maxImageExtent.width,
                          swap_chain_extent.width));
    swap_chain_extent.height =
        std::max(surface_capabilities.minImageExtent.height,
                 std::min(surface_capabilities.maxImageExtent.height,
                          swap_chain_extent.height));
    return swap_chain_extent;
  }
}

uint32_t TizenRendererVulkan::GetSwapChainNumImages(
    VkSurfaceCapabilitiesKHR& surface_capabilities) {
  const uint32_t maxImageCount = surface_capabilities.maxImageCount;
  const uint32_t minImageCount = surface_capabilities.minImageCount;
  uint32_t desiredImageCount = minImageCount + 1;

  // According to section 30.5 of VK 1.1, maxImageCount of zero means "that
  // there is no limit on the number of images, though there may be limits
  // related to the total amount of memory used by presentable images."
  if (maxImageCount != 0 && desiredImageCount > maxImageCount) {
    desiredImageCount = surface_capabilities.minImageCount;
  }
  return desiredImageCount;
}

VkPresentModeKHR TizenRendererVulkan::GetSwapChainPresentMode(
    std::vector<VkPresentModeKHR>& present_modes) {
  VkPresentModeKHR present_mode = present_modes[0];
  for (const auto& mode : present_modes) {
    if (mode == VK_PRESENT_MODE_FIFO_KHR) {
      present_mode = mode;
      break;
    }
  }
  return present_mode;
}

VkCompositeAlphaFlagBitsKHR TizenRendererVulkan::GetSwapChainCompositeAlpha(
    VkSurfaceCapabilitiesKHR& surface_capabilities) {
  if (surface_capabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
    return VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  } else {
    return VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  }
}

bool TizenRendererVulkan::InitializeSwapchain() {
  VkSwapchainKHR oldSwapchain = swapchain_;
  uint32_t format_count;
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(
          physical_device_, surface_, &format_count, nullptr) != VK_SUCCESS ||
      format_count == 0) {
    FT_LOG(Error)
        << "Error occurred during presentation surface formats enumeration";
    return false;
  }
  std::vector<VkSurfaceFormatKHR> formats(format_count);
  if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_,
                                           &format_count,
                                           formats.data()) != VK_SUCCESS) {
    FT_LOG(Error)
        << "Error occurred during presentation surface formats enumeration";
    return false;
  }
  VkSurfaceCapabilitiesKHR surface_capabilities;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          physical_device_, surface_, &surface_capabilities) != VK_SUCCESS) {
    FT_LOG(Error) << "Could not check presentation surface capabilities";
    return false;
  }

  uint32_t mode_count;
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(
          physical_device_, surface_, &mode_count, nullptr) != VK_SUCCESS) {
    FT_LOG(Error) << "Error occurred during presentation surface present modes "
                     "enumeration";
    return false;
  }
  std::vector<VkPresentModeKHR> modes(mode_count);
  if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device_, surface_,
                                                &mode_count, modes.data())) {
    FT_LOG(Error) << "Error occurred during presentation surface present modes "
                     "enumeration";
    return false;
  }

  surface_format_ = GetSwapChainFormat(formats);
  VkExtent2D clientSize = GetSwapChainExtent(surface_capabilities);
  uint32_t desiredImageCount = GetSwapChainNumImages(surface_capabilities);
  VkPresentModeKHR present_mode = GetSwapChainPresentMode(modes);
  VkCompositeAlphaFlagBitsKHR compositeAlpha =
      GetSwapChainCompositeAlpha(surface_capabilities);

  VkSwapchainCreateInfoKHR info{};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = surface_;
  info.minImageCount = desiredImageCount;
  info.imageFormat = surface_format_.format;
  info.imageColorSpace = surface_format_.colorSpace;
  info.imageExtent = clientSize;
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  info.queueFamilyIndexCount = 0;
  info.pQueueFamilyIndices = nullptr;
  info.preTransform = surface_capabilities.currentTransform;
  info.compositeAlpha = compositeAlpha;
  info.presentMode = present_mode;
  info.clipped = VK_TRUE;

  if (vkCreateSwapchainKHR(logical_device_, &info, nullptr, &swapchain_) !=
      VK_SUCCESS) {
    FT_LOG(Error) << "Could not create swap chain KHR";
    return false;
  }

  if (oldSwapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(logical_device_, oldSwapchain, nullptr);
  }

  // --------------------------------------------------------------------------
  // Fetch swapchain images
  // --------------------------------------------------------------------------

  uint32_t image_count;
  if (vkGetSwapchainImagesKHR(logical_device_, swapchain_, &image_count,
                              nullptr) != VK_SUCCESS) {
    FT_LOG(Error) << "Could not get swap chain images count";
    return false;
  }
  // swapchain_images_.reserve(image_count);
  swapchain_images_.resize(image_count);
  if (vkGetSwapchainImagesKHR(logical_device_, swapchain_, &image_count,
                              swapchain_images_.data()) != VK_SUCCESS) {
    FT_LOG(Error) << "Could not get swap chain images";
    return false;
  }

  // --------------------------------------------------------------------------
  // Record a command buffer for each of the images to be executed prior to
  // presenting.
  // --------------------------------------------------------------------------

  present_transition_buffers_.resize(swapchain_images_.size());

  VkCommandBufferAllocateInfo buffers_info{};
  buffers_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  buffers_info.commandPool = swapchain_command_pool_;
  buffers_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  buffers_info.commandBufferCount =
      static_cast<uint32_t>(present_transition_buffers_.size());

  if (vkAllocateCommandBuffers(logical_device_, &buffers_info,
                               present_transition_buffers_.data()) !=
      VK_SUCCESS) {
    FT_LOG(Error) << "Could not allocate command buffers for swapchain images!";
    return false;
  }
  for (size_t i = 0; i < swapchain_images_.size(); i++) {
    auto image = swapchain_images_[i];
    auto buffer = present_transition_buffers_[i];

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(buffer, &begin_info) != VK_SUCCESS) {
      FT_LOG(Error) << "Could not begin command buffer!";
      return false;
    }

    // Filament Engine hands back the image after writing to it
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    vkCmdPipelineBarrier(buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);
    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
      FT_LOG(Error) << "Could not end command buffer!";
      return false;
    }
  }
  return true;
}

bool TizenRendererVulkan::RecreateSwapChain() {
  vkDeviceWaitIdle(logical_device_);
  resize_pending_ = false;
  for (size_t i = 0; i < present_transition_buffers_.size(); ++i) {
    vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                         &present_transition_buffers_[i]);
  }
  DestroyCommandPool();
  if (!CreateCommandPool()) {
    FT_LOG(Error) << "Fail to create command pool!";
    return false;
  }
  if (!InitializeSwapchain()) {
    FT_LOG(Error) << "Fail to create swapchain!";
    return false;
  }
  return true;
}

void TizenRendererVulkan::DestroySurface() {
  if (surface_) {
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
  }
}

void TizenRendererVulkan::ResizeSurface(int32_t width, int32_t height) {
  if (width_ != width || height_ != height) {
    width_ = width;
    height_ = height;
    resize_pending_ = true;
  }
}
uint32_t TizenRendererVulkan::GetVersion() {
  return VK_API_VERSION_1_1;
}

FlutterVulkanInstanceHandle TizenRendererVulkan::GetInstanceHandle() {
  return instance_;
}

FlutterVulkanQueueHandle TizenRendererVulkan::GetQueueHandle() {
  return graphics_queue_;
}

FlutterVulkanPhysicalDeviceHandle
TizenRendererVulkan::GetPhysicalDeviceHandle() {
  return physical_device_;
}

FlutterVulkanDeviceHandle TizenRendererVulkan::GetDeviceHandle() {
  return logical_device_;
}

uint32_t TizenRendererVulkan::GetQueueIndex() {
  return graphics_queue_family_index_;
}

const char** TizenRendererVulkan::GetEnabledInstanceExtensions() {
  return enabled_instance_extensions_.data();
}

const char** TizenRendererVulkan::GetEnabledDeviceExtensions() {
  return enabled_device_extensions_.data();
}

void* TizenRendererVulkan::GetInstanceProcAddress(
    FlutterVulkanInstanceHandle instance,
    const char* name) {
  return reinterpret_cast<void*>(
      vkGetInstanceProcAddr(reinterpret_cast<VkInstance>(instance), name));
}

FlutterVulkanImage TizenRendererVulkan::GetNextImage(
    const FlutterFrameInfo* frameInfo) {
  if (resize_pending_) {
    RecreateSwapChain();
  }
  VkResult result;
  do {
    result = vkAcquireNextImageKHR(logical_device_, swapchain_, UINT64_MAX,
                                   VK_NULL_HANDLE, image_ready_fence_,
                                   &last_image_index_);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      RecreateSwapChain();
    } else if (result == VK_SUBOPTIMAL_KHR) {
      break;
    }
  } while (result != VK_SUCCESS);

  vkWaitForFences(logical_device_, 1, &image_ready_fence_, VK_TRUE, UINT64_MAX);
  vkResetFences(logical_device_, 1, &image_ready_fence_);
  return {
      .struct_size = sizeof(FlutterVulkanImage),
      .image = reinterpret_cast<uint64_t>(swapchain_images_[last_image_index_]),
      .format = static_cast<uint32_t>(surface_format_.format),
  };
}

bool TizenRendererVulkan::Present(const FlutterVulkanImage* image) {
  VkPipelineStageFlags stage_flags =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submit_info{};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = VK_NULL_HANDLE;
  submit_info.pWaitDstStageMask = &stage_flags;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &present_transition_buffers_[last_image_index_];
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &present_transition_semaphore_;
  vkQueueSubmit(graphics_queue_, 1, &submit_info, VK_NULL_HANDLE);

  VkPresentInfoKHR present_info{};
  present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  present_info.waitSemaphoreCount = 1;
  present_info.pWaitSemaphores = &present_transition_semaphore_;
  present_info.swapchainCount = 1;
  present_info.pSwapchains = &swapchain_;
  present_info.pImageIndices = &last_image_index_;
  VkResult result = vkQueuePresentKHR(graphics_queue_, &present_info);

  if (result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
    RecreateSwapChain();
  }
  vkDeviceWaitIdle(logical_device_);
  return result == VK_SUCCESS;
}

size_t TizenRendererVulkan::GetEnabledInstanceExtensionCount() {
  return enabled_instance_extensions_.size();
}

size_t TizenRendererVulkan::GetEnabledDeviceExtensionCount() {
  return enabled_device_extensions_.size();
}

VkCommandBuffer TizenRendererVulkan::BeginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = swapchain_command_pool_;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(logical_device_, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                         &commandBuffer);
    FT_LOG(Error) << "Failed to begin one-time command buffer.";
    return VK_NULL_HANDLE;
  }

  return commandBuffer;
}

void TizenRendererVulkan::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    FT_LOG(Error) << "Failed to end one-time command buffer.";
    vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                         &commandBuffer);
    return;
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(graphics_queue_, 1, &submitInfo, VK_NULL_HANDLE) !=
      VK_SUCCESS) {
    FT_LOG(Error) << "Failed to submit one-time command buffer.";
    vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                         &commandBuffer);
    return;
  }
  vkQueueWaitIdle(graphics_queue_);

  vkFreeCommandBuffers(logical_device_, swapchain_command_pool_, 1,
                       &commandBuffer);
}

}  // namespace flutter
