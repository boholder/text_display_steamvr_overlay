/*
 * Copyright (C) 2025. Nyabsi <nyabsi@sovellus.cc>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "VulkanRenderer.h"

#include <spdlog/spdlog.h>

#include "VulkanUtils.h"

#include <ranges>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>

#include <openvr.h>

static const std::vector<std::string> INSTANCE_EXTS_FOR_NO_VR
    = {VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef WIN32
       "VK_KHR_win32_surface",
#endif
       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

static const std::vector<std::string> DEVICE_EXTS_FOR_NO_VR = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
    VK_KHR_MULTIVIEW_EXTENSION_NAME,
    VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
};

VulkanRenderer::VulkanRenderer()
{
    vulkan_instance_ = VK_NULL_HANDLE;
    vulkan_physical_device_ = VK_NULL_HANDLE;
    vulkan_queue_family_ = -1;
    vulkan_allocator_ = nullptr;
    vulkan_device_ = VK_NULL_HANDLE;
    vulkan_queue_ = VK_NULL_HANDLE;
    vulkan_descriptor_pool_ = VK_NULL_HANDLE;
    vulkan_pipeline_cache_ = VK_NULL_HANDLE;
    minimum_concurrent_image_count_ = 0;
    vulkan_instance_extensions_ = {};
    vulkan_instance_extensions_.clear();
    vulkan_device_extensions_ = {};
    vulkan_device_extensions_.clear();
    debug_report_ = VK_NULL_HANDLE;
    device_list_.clear();
    should_enable_dynamic_rendering_ = false;
    f_vkCmdBeginRenderingKHR = nullptr;
    f_vkCmdEndRenderingKHR = nullptr;

    should_rebuild_swapchain_ = std::vector<std::atomic<bool>>(2);
    should_rebuild_swapchain_.at(0) = false;
    should_rebuild_swapchain_.at(1) = false;

    overlays_.resize(2);
    overlays_.clear();
    overlays_.push_back(std::make_unique<Vulkan_Overlay>());
    overlays_.push_back(std::make_unique<Vulkan_Overlay>());
}

/**
 * - create vulkan instance
 * - choose a physical device
 * - create vulkan logical device
 * - get graphics queue
 * - create descriptor pool
 */
auto VulkanRenderer::Initialize() -> void
{
    VkResult vk_result = {};

    auto convert_str_to_char_arr = [](const std::vector<std::string>& extensions) -> std::vector<const char*>
    {
        std::vector<const char*> result;
        for (auto& extension : extensions)
            result.push_back(extension.data());
        return result;
    };

#ifdef NO_VR
    std::vector instance_extensions = convert_str_to_char_arr(INSTANCE_EXTS_FOR_NO_VR);
#else
    // prepare instance extensions
    vulkan_instance_extensions_ = vk_util::GetVulkanExtensionsRequiredByOpenVR(INSTANCE, nullptr);
    auto instance_extensions = convert_str_to_char_arr(vulkan_instance_extensions_);
#endif

#ifdef ENABLE_VULKAN_VALIDATION
    instance_extensions.push_back("VK_EXT_debug_report");
#endif

    VkInstanceCreateInfo instance_create_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = ( uint32_t ) instance_extensions.size(),
        .ppEnabledExtensionNames = instance_extensions.data(),
    };

#ifdef ENABLE_VULKAN_VALIDATION
    const char* enabled_layers[] = {"VK_LAYER_KHRONOS_validation"};
    instance_create_info.enabledLayerCount = 1;
    instance_create_info.ppEnabledLayerNames = enabled_layers;
#endif

    // create vulkan instance
    vk_result = vkCreateInstance(&instance_create_info, vulkan_allocator_, &vulkan_instance_);
    VK_VALIDATE_RESULT(vk_result);

#ifdef ENABLE_VULKAN_VALIDATION
    auto DebugReport = [](VkDebugReportFlagsEXT flags,
                          VkDebugReportObjectTypeEXT objectType,
                          uint64_t object,
                          size_t location,
                          int32_t messageCode,
                          const char* pLayerPrefix,
                          const char* pMessage,
                          void* pUserData) -> VkBool32
    {
        ( void ) flags;
        ( void ) object;
        ( void ) location;
        ( void ) messageCode;
        ( void ) pUserData;
        ( void ) pLayerPrefix;

        // FIXME: The clean_resources() in "Main.c" doesn't properly destroy all Vulkan resources
        // when destroying g_subtitle_window and g_dashboard_window (due to using multiple ImGui contexts),
        // result in debug reports when calling vkDestroyDevice() in VulkanRenderer::Destroy()
        // temp suppress these reports
        if (!std::string(pMessage).contains("has not been destroyed"))
        {
            spdlog::error("[Vulkan] Debug report from ObjectType: [{}], message: {}", static_cast<int>(objectType), pMessage);
        }

        return VK_FALSE;
    };

    auto f_vkCreateDebugReportCallbackEXT
        = ( PFN_vkCreateDebugReportCallbackEXT ) vkGetInstanceProcAddr(vulkan_instance_, "vkCreateDebugReportCallbackEXT");
    VkDebugReportCallbackCreateInfoEXT debug_report_create_info
        = {.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
           .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
           .pfnCallback = DebugReport,
           .pUserData = nullptr};

    vk_result = f_vkCreateDebugReportCallbackEXT(vulkan_instance_, &debug_report_create_info, vulkan_allocator_, &debug_report_);
    VK_VALIDATE_RESULT(vk_result);
#endif

    // choose a physical device to use
    uint32_t device_count = {};
    vk_result = vkEnumeratePhysicalDevices(vulkan_instance_, &device_count, nullptr);
    VK_VALIDATE_RESULT(vk_result);

    if (device_count < 0)
        std::exit(EXIT_FAILURE);

    device_list_.resize(device_count);
    vk_result = vkEnumeratePhysicalDevices(vulkan_instance_, &device_count, device_list_.data());
    VK_VALIDATE_RESULT(vk_result);

    // prefer discrete (i.e., dedicated) GPU
    // TO DO: multi device support
    for (VkPhysicalDevice& device : device_list_)
    {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(device, &properties);

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            vulkan_physical_device_ = device;
            continue;
        }

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            vulkan_physical_device_ = device;
            break;
        }
    }

    VkPhysicalDeviceProperties properties = {};
    vkGetPhysicalDeviceProperties(vulkan_physical_device_, &properties);

    spdlog::info(
        "[Vulkan] Using device [{}], discrete: [{}]",
        properties.deviceName,
        properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ? "Yes" : "No"
    );
    assert(vulkan_physical_device_ != VK_NULL_HANDLE);

    // find a graphics-capable queue family on the selected GPU
    // queue family is used to indicate how to render
    uint32_t family_prop_count = {};
    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device_, &family_prop_count, nullptr);

    std::vector<VkQueueFamilyProperties> queues_properties = {};
    queues_properties.resize(family_prop_count);

    vkGetPhysicalDeviceQueueFamilyProperties(vulkan_physical_device_, &family_prop_count, queues_properties.data());

    for (auto [idx, property] : std::views::enumerate(queues_properties))
    {
        if (property.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            vulkan_queue_family_ = static_cast<uint32_t>(idx);
            break;
        }
    }

    queues_properties.clear();
    assert(vulkan_queue_family_ != ( uint32_t ) -1);

#ifdef NO_VR
    vulkan_device_extensions_ = DEVICE_EXTS_FOR_NO_VR;
#else
    // get device extensions
    vulkan_device_extensions_ = vk_util::GetVulkanExtensionsRequiredByOpenVR(DEVICE, vulkan_physical_device_);
#endif

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    if (!vk_util::IsVulkanExtensionAvailable(DEVICE, vulkan_physical_device_, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
        std::exit(EXIT_FAILURE);

    vulkan_device_extensions_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
#endif

    const bool has_dynamic_rendering_support
        = vk_util::IsVulkanExtensionAvailable(DEVICE, vulkan_physical_device_, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
          && vk_util::IsVulkanExtensionAvailable(DEVICE, vulkan_physical_device_, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME)
          && vk_util::IsVulkanExtensionAvailable(DEVICE, vulkan_physical_device_, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);

    if (!has_dynamic_rendering_support)
        std::exit(EXIT_FAILURE);

    should_enable_dynamic_rendering_ = has_dynamic_rendering_support;
    vulkan_device_extensions_.insert(
        vulkan_device_extensions_.end(),
        {VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME}
    );

    auto device_extensions = convert_str_to_char_arr(vulkan_device_extensions_);

    // create logical device
    constexpr float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo device_queue_info
        = {.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
           .queueFamilyIndex = vulkan_queue_family_,
           .queueCount = 1,
           .pQueuePriorities = &queue_priority};

    VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
        .dynamicRendering = true,
    };

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &dynamic_rendering_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_info,
        .enabledExtensionCount = ( uint32_t ) device_extensions.size(),
        .ppEnabledExtensionNames = device_extensions.data(),
    };

    vk_result = vkCreateDevice(vulkan_physical_device_, &device_create_info, vulkan_allocator_, &vulkan_device_);
    VK_VALIDATE_RESULT(vk_result);

    vkGetDeviceQueue(vulkan_device_, vulkan_queue_family_, 0, &vulkan_queue_);

    VkDescriptorPoolSize pool_sizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE},
    };

    VkDescriptorPoolCreateInfo pool_info
        = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
           .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
           .maxSets = 0,
           .poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes)),
           .pPoolSizes = pool_sizes};

    for (VkDescriptorPoolSize const& pool_size : pool_sizes)
        pool_info.maxSets += pool_size.descriptorCount;

    vk_result = vkCreateDescriptorPool(vulkan_device_, &pool_info, vulkan_allocator_, &vulkan_descriptor_pool_);
    VK_VALIDATE_RESULT(vk_result);

    this->f_vkCmdBeginRenderingKHR = ( PFN_vkCmdBeginRenderingKHR ) vkGetInstanceProcAddr(vulkan_instance_, "vkCmdBeginRenderingKHR");
    assert(f_vkCmdBeginRenderingKHR != nullptr);
    this->f_vkCmdEndRenderingKHR = ( PFN_vkCmdEndRenderingKHR ) vkGetInstanceProcAddr(vulkan_instance_, "vkCmdEndRenderingKHR");
    assert(f_vkCmdEndRenderingKHR != nullptr);
}

auto VulkanRenderer::SetupWindow(VulkanWindow* window, VkSurfaceKHR surface, uint32_t width, uint32_t height) -> void
{
    VkResult vk_result = {};

    window->surface = surface;

    VkBool32 result = {};
    vk_result = vkGetPhysicalDeviceSurfaceSupportKHR(
        vulkan_physical_device_, vulkan_queue_family_, window->surface, &result
    ); // Check for WSI support
    VK_VALIDATE_RESULT(vk_result);

    if (result != VK_TRUE)
    {
        spdlog::error("No WSI support on physical device 0");
        exit(-1);
    }

    // request R8G8B8A8 (RGBA, instead of ARGB) format for OpenVR
    // All compatible formats can be found at https://github.com/ValveSoftware/openvr/wiki/Vulkan#image-formats
    VkSurfaceFormatKHR surface_format = {
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR // make sure colour space is non linear otherwise it will not render on AMD GPUs
    };

    // TO DO: validate if surface format is available on the current hardware.

    uint32_t mode_count = {};
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_physical_device_, window->surface, &mode_count, nullptr);

    std::vector<VkPresentModeKHR> m_modes = {};
    m_modes.resize(mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(vulkan_physical_device_, window->surface, &mode_count, m_modes.data());

    auto present_mode_to_string = [](VkPresentModeKHR& mode) -> std::string
    {
        switch (mode)
        {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return "VK_PRESENT_MODE_MAILBOX_KHR";
        case VK_PRESENT_MODE_FIFO_KHR:
            return "VK_PRESENT_MODE_FIFO_KHR";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
            return "VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR";
        case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
            return "VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR";
        default:
            return "N/A";
        }
    };

    spdlog::info("Available present modes:");

    for (auto& mode : m_modes)
    {
        spdlog::info("\t- {}", present_mode_to_string(mode));
    }

    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;

    auto mailbox = std::find(m_modes.begin(), m_modes.end(), VK_PRESENT_MODE_MAILBOX_KHR);
    if (mailbox != m_modes.end())
        present_mode = VK_PRESENT_MODE_MAILBOX_KHR;

    auto relaxed = std::find(m_modes.begin(), m_modes.end(), VK_PRESENT_MODE_FIFO_RELAXED_KHR);
    if (relaxed != m_modes.end() && mailbox == m_modes.end())
        present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;

    spdlog::info("Selected: [{}]", present_mode_to_string(present_mode));

    window->surface_format = surface_format;
    window->present_mode = present_mode;
    window->clear_enable = true;

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    this->SetupSwapchain(window, width, height);
#endif
}

auto VulkanRenderer::SetupOverlay(uint32_t index, uint32_t width, uint32_t height, VkSurfaceFormatKHR format) -> void
{
    VkResult vk_result = {};

    auto* ovl = overlays_.at(index).get();

    ovl->width = width;
    ovl->height = height;
    ovl->texture_format = format;
    ovl->clear_enable = true;

    VkCommandPoolCreateInfo command_pool_create_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = 0,
        .queueFamilyIndex = vulkan_queue_family_,
    };

    vk_result = vkCreateCommandPool(vulkan_device_, &command_pool_create_info, vulkan_allocator_, &ovl->command_pool);
    VK_VALIDATE_RESULT(vk_result);

    VkCommandBufferAllocateInfo command_buffer_allocate_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = ovl->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    vk_result = vkAllocateCommandBuffers(vulkan_device_, &command_buffer_allocate_info, &ovl->command_buffer);
    VK_VALIDATE_RESULT(vk_result);

    VkFenceCreateInfo fence_create_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    vk_result = vkCreateFence(vulkan_device_, &fence_create_info, vulkan_allocator_, &ovl->fence);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkWaitForFences(vulkan_device_, 1, &ovl->fence, VK_TRUE, UINT64_MAX);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetFences(vulkan_device_, 1, &ovl->fence);
    VK_VALIDATE_RESULT(vk_result);

    vkGetDeviceQueue(vulkan_device_, vulkan_queue_family_, 0, &ovl->queue);

    VkCommandBufferBeginInfo begin_info
        = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vk_result = vkBeginCommandBuffer(ovl->command_buffer, &begin_info);
    VK_VALIDATE_RESULT(vk_result);

    VkImageCreateInfo image_create_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = ovl->texture_format.format,
        .extent =
        {
            .width = ovl->width,
            .height = ovl->height,
            .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    vk_result = vkCreateImage(vulkan_device_, &image_create_info, nullptr, &ovl->texture);
    VK_VALIDATE_RESULT(vk_result);

    auto find_memory_type_index = [&](uint32_t type, VkMemoryPropertyFlags properties) -> uint32_t
    {
        VkPhysicalDeviceMemoryProperties memory_requirements = {};
        vkGetPhysicalDeviceMemoryProperties(vulkan_physical_device_, &memory_requirements);

        for (uint32_t i = 0; i < memory_requirements.memoryTypeCount; i++)
        {
            if ((type & (1 << i)) && (memory_requirements.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type!");
    };

    VkMemoryRequirements memory_requirements = {};
    vkGetImageMemoryRequirements(vulkan_device_, ovl->texture, &memory_requirements);

    VkMemoryAllocateInfo memory_alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = find_memory_type_index(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    vk_result = vkAllocateMemory(vulkan_device_, &memory_alloc_info, nullptr, &ovl->texture_memory);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkBindImageMemory(vulkan_device_, ovl->texture, ovl->texture_memory, 0);
    VK_VALIDATE_RESULT(vk_result);

    VkImageViewCreateInfo image_view_info =
    {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = ovl->texture,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = ovl->texture_format.format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vk_result = vkCreateImageView(vulkan_device_, &image_view_info, vulkan_allocator_, &ovl->texture_view);
    VK_VALIDATE_RESULT(vk_result);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ovl->texture,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(
        ovl->command_buffer,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );

    vk_result = vkEndCommandBuffer(ovl->command_buffer);
    VK_VALIDATE_RESULT(vk_result);

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &ovl->command_buffer,
    };

    vk_result = vkQueueSubmit(ovl->queue, 1, &submit_info, ovl->fence);
    VK_VALIDATE_RESULT(vk_result);
}

auto VulkanRenderer::SetupSwapchain(VulkanWindow* window, uint32_t width, uint32_t height) -> void
{
    VkResult vk_result = {};
    VkSwapchainKHR old_swapchain = window->swapchain;

    vk_result = vkQueueWaitIdle(vulkan_queue_);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkDeviceWaitIdle(vulkan_device_);
    VK_VALIDATE_RESULT(vk_result);

    window->swapchain = VK_NULL_HANDLE;

    this->DestroyFrames(window);
    window->image_count = 0;

    if (window->render_pass)
        vkDestroyRenderPass(vulkan_device_, window->render_pass, vulkan_allocator_);
    if (window->pipeline)
        vkDestroyPipeline(vulkan_device_, window->pipeline, vulkan_allocator_);

    VkSurfaceCapabilitiesKHR cap = {};
    vk_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vulkan_physical_device_, window->surface, &cap);
    VK_VALIDATE_RESULT(vk_result);

    // currentExtent will be (0,0) if the surface is minimized,
    // cause error: vkCreateSwapchainKHR(): pCreateInfo->imageExtent (width = 0, height = 0) is invalid.
    if (cap.currentExtent.width == 0 || cap.currentExtent.height == 0)
    {
        if (old_swapchain)
        {
            vkDestroySwapchainKHR(vulkan_device_, old_swapchain, vulkan_allocator_);
        }
        should_rebuild_swapchain_.at(window->index) = true;
        return;
    }

    auto set_minimum_concurrent_image_count = [](VkPresentModeKHR mode)
    {
        switch (mode)
        {
        case VK_PRESENT_MODE_MAILBOX_KHR:
            return 3;
        case VK_PRESENT_MODE_FIFO_KHR:
            [[fallthrough]];
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
            return 2;
        case VK_PRESENT_MODE_IMMEDIATE_KHR:
            return 1;
        default:
            return 1;
        }
    };

    if (minimum_concurrent_image_count_ == 0)
    {
        minimum_concurrent_image_count_ = set_minimum_concurrent_image_count(window->present_mode);
    }

    uint32_t min_image_count = minimum_concurrent_image_count_;

    if (window->present_mode != VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR
        && window->present_mode != VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR && cap.minImageCount > min_image_count)
        min_image_count = cap.minImageCount;

    if (min_image_count > cap.maxImageCount)
        min_image_count = cap.maxImageCount;

    window->width = width;
    window->height = height;

    // (0xFFFFFFFF, 0xFFFFFFFF) indicating that the surface size will be determined by the extent of a swapchain targeting the surface.
    if (cap.currentExtent.width != 0xffffffff && cap.currentExtent.height != 0xffffffff)
    {
        window->width = cap.currentExtent.width;
        window->height = cap.currentExtent.height;
    }

    VkSurfaceTransformFlagBitsKHR surface_transform_flags
        = cap.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : cap.currentTransform;

    VkCompositeAlphaFlagBitsKHR composite_alpha;
    if (cap.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
        composite_alpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    else if (cap.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
        composite_alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    else
        IM_ASSERT(false && "No supported composite alpha mode found!");

    VkSwapchainCreateInfoKHR const swapchain_create_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = window->surface,
        .minImageCount = min_image_count,
        .imageFormat = window->surface_format.format,
        .imageColorSpace = window->surface_format.colorSpace,
        .imageExtent = {.width = window->width, .height = window->height},
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surface_transform_flags,
        .compositeAlpha = composite_alpha,
        .presentMode = window->present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };

    vk_result = vkCreateSwapchainKHR(vulkan_device_, &swapchain_create_info, vulkan_allocator_, &window->swapchain);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkGetSwapchainImagesKHR(vulkan_device_, window->swapchain, &window->image_count, nullptr);
    VK_VALIDATE_RESULT(vk_result);

    VkImage backbuffers[16] = {};

    assert(window->image_count >= minimum_concurrent_image_count_);
    assert(window->image_count < 16);

    vk_result = vkGetSwapchainImagesKHR(vulkan_device_, window->swapchain, &window->image_count, backbuffers);
    VK_VALIDATE_RESULT(vk_result);

    window->semaphore_count = window->image_count + 1;
    window->frames.resize(window->image_count);
    window->semaphores.resize(window->semaphore_count);

    memset(window->semaphores.data(), 0x0, window->semaphores.size() * sizeof(Vulkan_FrameSemaphore));
    memset(window->frames.data(), 0x0, window->frames.size() * sizeof(Vulkan_Frame));

    if (old_swapchain)
        vkDestroySwapchainKHR(vulkan_device_, old_swapchain, vulkan_allocator_);

    for (uint32_t idx = 0; idx < window->semaphore_count; idx++)
    {
        Vulkan_FrameSemaphore* fsd = &window->semaphores[idx];

        VkSemaphoreCreateInfo semaphore_create_info = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        vk_result = vkCreateSemaphore(vulkan_device_, &semaphore_create_info, vulkan_allocator_, &fsd->image_acquired_semaphore);
        VK_VALIDATE_RESULT(vk_result);

        vk_result = vkCreateSemaphore(vulkan_device_, &semaphore_create_info, vulkan_allocator_, &fsd->render_complete_semaphore);
        VK_VALIDATE_RESULT(vk_result);
    }

    for (uint32_t idx = 0; idx < window->image_count; idx++)
    {
        Vulkan_Frame* fd = &window->frames[idx];

        fd->backbuffer = backbuffers[idx];
        VkImageViewCreateInfo image_view_info =
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = fd->backbuffer,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = window->surface_format.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A,
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vk_result = vkCreateImageView(vulkan_device_, &image_view_info, vulkan_allocator_, &fd->backbuffer_view);
        VK_VALIDATE_RESULT(vk_result);

        VkCommandPoolCreateInfo command_pool_create_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = 0,
            .queueFamilyIndex = vulkan_queue_family_,
        };

        vk_result = vkCreateCommandPool(vulkan_device_, &command_pool_create_info, vulkan_allocator_, &fd->command_pool);
        VK_VALIDATE_RESULT(vk_result);

        VkCommandBufferAllocateInfo command_buffer_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = fd->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        vk_result = vkAllocateCommandBuffers(vulkan_device_, &command_buffer_allocate_info, &fd->command_buffer);
        VK_VALIDATE_RESULT(vk_result);

        VkFenceCreateInfo fence_create_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        vk_result = vkCreateFence(vulkan_device_, &fence_create_info, vulkan_allocator_, &fd->fence);
        VK_VALIDATE_RESULT(vk_result);

        VkCommandBufferBeginInfo begin_info
            = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

        vk_result = vkBeginCommandBuffer(fd->command_buffer, &begin_info);
        VK_VALIDATE_RESULT(vk_result);

        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = fd->backbuffer,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
        };

        vkCmdPipelineBarrier(
            fd->command_buffer,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier
        );

        vk_result = vkEndCommandBuffer(fd->command_buffer);
        VK_VALIDATE_RESULT(vk_result);

        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &fd->command_buffer,
        };

        vk_result = vkWaitForFences(vulkan_device_, 1, &fd->fence, VK_TRUE, UINT64_MAX);
        VK_VALIDATE_RESULT(vk_result);

        vk_result = vkResetFences(vulkan_device_, 1, &fd->fence);
        VK_VALIDATE_RESULT(vk_result);

        vk_result = vkQueueSubmit(vulkan_queue_, 1, &submit_info, fd->fence);
        VK_VALIDATE_RESULT(vk_result);
    }

    should_rebuild_swapchain_.at(window->index) = false;
}

/**
 *  Render ImDrawData to a VulkanWindow using Vulkan's dynamic rendering pipeline.
 *
 *  - get the next swapchain image (frame)
 *  - wait until it’s free (waiting for the fence)
 *  - reset fence and command pool
 *  - translate ImDrawData into command buffer
 *  - submit commands to the GPU, use the fence to subscribe the result
 *
 *  @param draw_data  ImGui::GetDrawData()
 *  @param window     ImGuiWindow::WindowData()
 */
auto VulkanRenderer::RenderWindow(ImDrawData* draw_data, VulkanWindow* window) -> void
{
    if (window->is_minimized)
        return;

    VkResult vk_result = {};

    VkSemaphore image_acquired_semaphore = window->semaphores[window->semaphore_index].image_acquired_semaphore;
    VkSemaphore render_complete_semaphore = window->semaphores[window->semaphore_index].render_complete_semaphore;

    // get the next image from swapchain
    // vulkan writes the image to window->frames
    vk_result = vkAcquireNextImageKHR(
        vulkan_device_, window->swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &window->frame_index
    );
    // ref: https://vulkan-tutorial.com/Drawing_a_triangle/Swap_chain_recreation#page_Suboptimal-or-out-of-date-swap-chain
    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR)
        should_rebuild_swapchain_.at(window->index) = true;
    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR)
        return;
    if (vk_result != VK_SUBOPTIMAL_KHR)
        VK_VALIDATE_RESULT(vk_result);

    Vulkan_Frame const* fd = &window->frames[window->frame_index];

    vk_result = vkWaitForFences(vulkan_device_, 1, &fd->fence, VK_TRUE, UINT64_MAX);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetFences(vulkan_device_, 1, &fd->fence);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetCommandPool(vulkan_device_, fd->command_pool, 0);
    VK_VALIDATE_RESULT(vk_result);

    const VkCommandBufferBeginInfo buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vk_result = vkBeginCommandBuffer(fd->command_buffer, &buffer_begin_info);
    VK_VALIDATE_RESULT(vk_result);

    const VkRenderingAttachmentInfoKHR color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = fd->backbuffer_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE_KHR,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = window->clear_enable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = window->clear_value,
    };

    const VkRenderingInfoKHR rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .flags = 0,
        .renderArea =
        {
            .extent =
            {
                .width = window->width,
                .height = window->height,
            },
        },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    // before calling vkCmdBeginRendering, add a barrier to
    // translate the image layout from PRESENT_SRC_KHR to COLOR_ATTACHMENT_OPTIMAL
    const VkImageMemoryBarrier render_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = fd->backbuffer,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(
        fd->command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &render_barrier
    );

    f_vkCmdBeginRenderingKHR(fd->command_buffer, &rendering_info);
    // the ImGui backend translates GUI widgets into Vulkan draw commands
    ImGui_ImplVulkan_RenderDrawData(draw_data, fd->command_buffer);
    f_vkCmdEndRenderingKHR(fd->command_buffer);

    // after rendering complete, translate image layout back
    const VkImageMemoryBarrier present_barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = 0,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = fd->backbuffer,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(
        fd->command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &present_barrier
    );

    vk_result = vkEndCommandBuffer(fd->command_buffer);
    VK_VALIDATE_RESULT(vk_result);

    // wait at the color attachment output stage before executing the command buffer
    VkPipelineStageFlags const wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    const VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_acquired_semaphore,
        .pWaitDstStageMask = &wait_stage_mask,
        .commandBufferCount = 1,
        .pCommandBuffers = &fd->command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_complete_semaphore,
    };

    vk_result = vkQueueSubmit(vulkan_queue_, 1, &submit_info, fd->fence);
    VK_VALIDATE_RESULT(vk_result);
}

auto VulkanRenderer::RenderOverlay(uint32_t index, ImDrawData* draw_data, VrOverlay*& overlay) -> void
{
    if (!overlay->IsVisible())
        return;

    VkResult vk_result = {};

    auto* ovl = overlays_.at(index).get();

    /// NOTE: suboptimal
    vk_util::set_vk_clear_value_background_color(ovl->clear_value, ImVec4(0.45f, 0.55f, 0.60f, 1.00f));

    VkCommandBufferBeginInfo buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };

    VkRenderingAttachmentInfoKHR color_attachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
        .imageView = ovl->texture_view,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .resolveMode = VK_RESOLVE_MODE_NONE_KHR,
        .resolveImageView = VK_NULL_HANDLE,
        .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .loadOp = ovl->clear_enable ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = ovl->clear_value,
    };

    VkRenderingInfoKHR rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
        .flags = 0,
        .renderArea =
        {
            .extent =
            {
                .width = ovl->width,
                .height = ovl->height,
            },
        },
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment,
        .pDepthAttachment = nullptr,
        .pStencilAttachment = nullptr,
    };

    vk_result = vkWaitForFences(vulkan_device_, 1, &ovl->fence, VK_TRUE, UINT64_MAX);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetFences(vulkan_device_, 1, &ovl->fence);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetCommandPool(vulkan_device_, ovl->command_pool, 0);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkBeginCommandBuffer(ovl->command_buffer, &buffer_begin_info);
    VK_VALIDATE_RESULT(vk_result);

    f_vkCmdBeginRenderingKHR(ovl->command_buffer, &rendering_info);
    ImGui_ImplVulkan_RenderDrawData(draw_data, ovl->command_buffer);
    f_vkCmdEndRenderingKHR(ovl->command_buffer);

    VkImageMemoryBarrier barrier_optimal = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ovl->texture,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(
        ovl->command_buffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier_optimal
    );

    vk_result = vkEndCommandBuffer(ovl->command_buffer);
    VK_VALIDATE_RESULT(vk_result);

    VkSubmitInfo submit_info_barrier = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &ovl->command_buffer,
    };

    vk_result = vkQueueSubmit(ovl->queue, 1, &submit_info_barrier, ovl->fence);
    VK_VALIDATE_RESULT(vk_result);

    vr::VRVulkanTextureData_t vulkanTexure = {
        .m_nImage = ( uintptr_t ) ovl->texture,
        .m_pDevice = vulkan_device_,
        .m_pPhysicalDevice = vulkan_physical_device_,
        .m_pInstance = vulkan_instance_,
        .m_pQueue = vulkan_queue_,
        .m_nQueueFamilyIndex = ( uint32_t ) vulkan_queue_family_,
        .m_nWidth = ovl->width,
        .m_nHeight = ovl->height,
        .m_nFormat = ( uint32_t ) ovl->texture_format.format,
        .m_nSampleCount = VK_SAMPLE_COUNT_1_BIT,
    };

    vr::Texture_t vrTexture = {
        .handle = ( void* ) &vulkanTexure,
        .eType = vr::TextureType_Vulkan,
        .eColorSpace = vr::ColorSpace_Auto,
    };

    try
    {
        overlay->SetTexture(vrTexture);
    }
    catch (std::exception& ex)
    {
        spdlog::error("[OpenVR] Failed to set overlay texture: {}", ex.what());
        return;
    }

    vk_result = vkWaitForFences(vulkan_device_, 1, &ovl->fence, VK_TRUE, UINT64_MAX);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetFences(vulkan_device_, 1, &ovl->fence);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkResetCommandPool(vulkan_device_, ovl->command_pool, 0);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkBeginCommandBuffer(ovl->command_buffer, &buffer_begin_info);
    VK_VALIDATE_RESULT(vk_result);

    VkImageMemoryBarrier barrier_restore = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = ovl->texture,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vkCmdPipelineBarrier(
        ovl->command_buffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier_restore
    );

    vk_result = vkEndCommandBuffer(ovl->command_buffer);
    VK_VALIDATE_RESULT(vk_result);

    vk_result = vkQueueSubmit(ovl->queue, 1, &submit_info_barrier, ovl->fence);
    VK_VALIDATE_RESULT(vk_result);
}

auto VulkanRenderer::Present(VulkanWindow* window) -> void
{
    if (should_rebuild_swapchain_.at(window->index) || window->is_minimized)
        return;

    VkResult vk_result = {};

    VkSemaphore render_complete_semaphore = window->semaphores[window->semaphore_index].render_complete_semaphore;

    VkPresentInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &render_complete_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &window->swapchain,
        .pImageIndices = &window->frame_index,
    };

    vk_result = vkQueuePresentKHR(vulkan_queue_, &info);

    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR || vk_result == VK_SUBOPTIMAL_KHR)
        should_rebuild_swapchain_.at(window->index) = true;

    if (vk_result == VK_ERROR_OUT_OF_DATE_KHR)
        return;

    if (vk_result != VK_SUBOPTIMAL_KHR)
        VK_VALIDATE_RESULT(vk_result);

    window->semaphore_index = (window->semaphore_index + 1) % window->semaphore_count;
}

auto VulkanRenderer::DestroyWindow(VulkanWindow* window) const -> void
{
    this->DestroyFrames(window);

    vkDestroyPipeline(vulkan_device_, window->pipeline, vulkan_allocator_);
    vkDestroyRenderPass(vulkan_device_, window->render_pass, vulkan_allocator_);
    vkDestroySwapchainKHR(vulkan_device_, window->swapchain, vulkan_allocator_);
    vkDestroySurfaceKHR(vulkan_instance_, window->surface, vulkan_allocator_);
}

auto VulkanRenderer::DestroyFrames(VulkanWindow* window) const -> void
{
    VkResult vk_result = {};
    vk_result = vkQueueWaitIdle(vulkan_queue_);
    VK_VALIDATE_RESULT(vk_result);

    for (uint32_t idx = 0; idx < window->semaphore_count; idx++)
    {
        Vulkan_FrameSemaphore* fsd = &window->semaphores[idx];

        vkDestroySemaphore(vulkan_device_, fsd->image_acquired_semaphore, vulkan_allocator_);
        vkDestroySemaphore(vulkan_device_, fsd->render_complete_semaphore, vulkan_allocator_);

        fsd->image_acquired_semaphore = VK_NULL_HANDLE;
        fsd->render_complete_semaphore = VK_NULL_HANDLE;
    }

    for (uint32_t idx = 0; idx < window->image_count; idx++)
    {
        Vulkan_Frame* fd = &window->frames[idx];

        vkDestroyFence(vulkan_device_, fd->fence, vulkan_allocator_);
        vkFreeCommandBuffers(vulkan_device_, fd->command_pool, 1, &fd->command_buffer);
        vkDestroyCommandPool(vulkan_device_, fd->command_pool, vulkan_allocator_);
        vkDestroyImageView(vulkan_device_, fd->backbuffer_view, vulkan_allocator_);
        vkDestroyFramebuffer(vulkan_device_, fd->framebuffer, vulkan_allocator_);

        fd->command_pool = VK_NULL_HANDLE;
        fd->command_buffer = VK_NULL_HANDLE;
        fd->fence = VK_NULL_HANDLE;
        fd->backbuffer = VK_NULL_HANDLE;
        fd->backbuffer_view = VK_NULL_HANDLE;
        fd->framebuffer = VK_NULL_HANDLE;
    }
}

auto VulkanRenderer::DestroyOverlay(Vulkan_Overlay* vulkan_overlay) const -> void
{
    VkResult vk_result = {};
    vk_result = vkQueueWaitIdle(vulkan_queue_);
    VK_VALIDATE_RESULT(vk_result);

    vkDestroyFence(vulkan_device_, vulkan_overlay->fence, vulkan_allocator_);
    vkFreeCommandBuffers(vulkan_device_, vulkan_overlay->command_pool, 1, &vulkan_overlay->command_buffer);
    vkDestroyCommandPool(vulkan_device_, vulkan_overlay->command_pool, vulkan_allocator_);

    vkDestroyImage(vulkan_device_, vulkan_overlay->texture, vulkan_allocator_);
    vkFreeMemory(vulkan_device_, vulkan_overlay->texture_memory, vulkan_allocator_);
    vkDestroyImageView(vulkan_device_, vulkan_overlay->texture_view, vulkan_allocator_);

    vulkan_overlay->fence = VK_NULL_HANDLE;
    vulkan_overlay->command_pool = VK_NULL_HANDLE;
    vulkan_overlay->command_buffer = VK_NULL_HANDLE;
    vulkan_overlay->texture = VK_NULL_HANDLE;
    vulkan_overlay->texture_memory = VK_NULL_HANDLE;
    vulkan_overlay->texture_view = VK_NULL_HANDLE;
}

auto VulkanRenderer::Destroy() -> void
{
    VkResult vk_result = {};

    vk_result = vkQueueWaitIdle(vulkan_queue_);
    VK_VALIDATE_RESULT(vk_result);

#ifdef ENABLE_VULKAN_VALIDATION
    auto f_vkDestroyDebugReportCallbackEXT
        = ( PFN_vkDestroyDebugReportCallbackEXT ) vkGetInstanceProcAddr(vulkan_instance_, "vkDestroyDebugReportCallbackEXT");
    f_vkDestroyDebugReportCallbackEXT(vulkan_instance_, nullptr, vulkan_allocator_);
#endif

    vkDestroyDescriptorPool(vulkan_device_, vulkan_descriptor_pool_, vulkan_allocator_);
    vkDestroyDevice(vulkan_device_, vulkan_allocator_);
    vkDestroyInstance(vulkan_instance_, vulkan_allocator_);
}
