/*
 * Copyright (C) 2025. Nyabsi <nyabsi@sovellus.cc>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vector>
#include <sstream>

#include <vulkan/vulkan.h>
#include <openvr.h>

#include <spdlog/spdlog.h>

#define VK_VALIDATE_RESULT(e)                                                                                                              \
    if ((e) != VK_SUCCESS)                                                                                                                 \
        SPDLOG_ERROR("[Vulkan] Vulkan operation result is not VK_SUCCESS: {}", ( int ) (e));                                               \
    if ((e) > 0)                                                                                                                           \
        assert(e);

enum VulkanExtensionType
{
    DEVICE,
    INSTANCE
};

namespace vk_util
{

static bool
    IsVulkanExtensionAvailable(const VulkanExtensionType type, const VkPhysicalDevice& physical_device, const std::string& extension)
{
    auto get_properties = [=](const VkPhysicalDevice& d, uint32_t& c, VkExtensionProperties* p)
    {
        if (type == INSTANCE)
        {
            vkEnumerateInstanceExtensionProperties(nullptr, &c, p);
        }
        else
        {
            vkEnumerateDeviceExtensionProperties(d, nullptr, &c, p);
        }
    };

    uint32_t extension_properties_count = {};
    std::vector<VkExtensionProperties> extension_properties = {};

    get_properties(physical_device, extension_properties_count, nullptr);

    if (extension_properties_count > 0)
    {
        extension_properties.resize(extension_properties_count);
        get_properties(physical_device, extension_properties_count, extension_properties.data());
    }
    else
    {
        SPDLOG_ERROR("[Vulkan] No Vulkan {} extension properties found in local, exit", type == INSTANCE ? "instance" : "device");
        std::exit(EXIT_FAILURE);
    }

    for (const VkExtensionProperties& p : extension_properties)
    {
        if (strcmp(p.extensionName, extension.c_str()) == 0)
            return true;
    }
    return false;
}

static std::vector<std::string> GetVulkanExtensionsRequiredByOpenVR(const VulkanExtensionType type, const VkPhysicalDevice& device)
{
    auto get_extensions = [=](VkPhysicalDevice_T* d, char* b, uint32_t s)
    {
        if (type == INSTANCE)
        {
            return vr::VRCompositor()->GetVulkanInstanceExtensionsRequired(b, s);
        }
        else
        {
            return vr::VRCompositor()->GetVulkanDeviceExtensionsRequired(d, b, s);
        }
    };

    std::vector<std::string> result{};

    if (!vr::VRCompositor())
    {
        std::exit(EXIT_FAILURE);
    }

    uint32_t buffer_len = get_extensions(device, nullptr, 0);
    if (buffer_len > 0)
    {
        // get required extensions into buffer
        std::vector<char> buffer(buffer_len + 1);
        get_extensions(device, buffer.data(), buffer_len);
        buffer[buffer_len] = '\0';

        // check if each extension is available
        std::string token{};
        std::istringstream token_stream(buffer.data());
        while (std::getline(token_stream, token, ' '))
        {
            if (IsVulkanExtensionAvailable(type, device, token))
            {
                result.push_back(token);
            }
            else
            {
                SPDLOG_ERROR(
                    "[Vulkan] [{}] {} extension asked by OpenVR was NOT available, exit", type == INSTANCE ? "instance" : "device", token
                );
                std::exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        SPDLOG_ERROR("[Vulkan] NO Vulkan {} extension asked by OpenVR, exit", type == INSTANCE ? "instance" : "device");
        std::exit(EXIT_FAILURE);
    }

    return result;
}

static void set_vk_clear_value_background_color(VkClearValue& clear_value, const ImVec4& color)
{
    clear_value.color.float32[0] = color.x * color.w;
    clear_value.color.float32[1] = color.y * color.w;
    clear_value.color.float32[2] = color.z * color.w;
    clear_value.color.float32[3] = color.w;
}
} // namespace vk_util
