/*
 * Copyright (C) 2025. Nyabsi <nyabsi@sovellus.cc>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cstdio>
#include <cstdlib>

#include <sstream>
#include <fstream>
#include <vector>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_vulkan.h>

#include <spdlog/spdlog.h>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <openvr.h>

#include "VulkanRenderer.h"
#include "VulkanUtils.h"

#include "ImGuiWindow.h"
#include "ImGuiOverlayWindow.h"

#include "VrOverlay.h"
#include "VrUtils.h"

#include "constants.h"

#include "backends/imgui_impl_openvr.h"

#ifdef _WIN32
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif

static VulkanRenderer* g_vulkanRenderer = new VulkanRenderer();
static ImGuiWindow* g_imGuiWindow = new ImGuiWindow();
static ImGuiOverlayWindow* g_ImGuiOverlayWindow = new ImGuiOverlayWindow();
static VrOverlay* g_window_overlay = new VrOverlay();
static VrOverlay* g_dashboard_overlay = new VrOverlay();

static uint64_t g_last_frame_time = SDL_GetTicksNS();
static float g_hmd_refresh_rate = 24.0F;
static bool g_ticking = true;

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720

static void UpdateApplicationRefreshRate()
{
    try
    {
        auto hmd_properties = VrTrackedDeviceProperties::FromDeviceIndex(vr::k_unTrackedDeviceIndex_Hmd);
        hmd_properties.CheckConnection();
        g_hmd_refresh_rate = hmd_properties.GetFloat(vr::Prop_DisplayFrequency_Float);
    }
    catch (std::exception& ex)
    {
        spdlog::error("Failed to update HMD refresh rate: {}", ex.what());
        if (g_hmd_refresh_rate == 24.0F)
            std::exit(EXIT_FAILURE);
    }
}

static void create_window_overlay()
{
    g_window_overlay->Create(vr::VROverlayType_World, WINDOW_KEY, WINDOW_NAME);

    g_window_overlay->SetInputMethod(vr::VROverlayInputMethod_Mouse);
    g_window_overlay->SetWidth(1.0F);

    g_window_overlay->EnableFlag(vr::VROverlayFlags_SendVRDiscreteScrollEvents);
    g_window_overlay->EnableFlag(vr::VROverlayFlags_EnableClickStabilization);
    g_window_overlay->EnableFlag(vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible);

    // Origin relative offset
    glm::vec3 const position = { 0.0F, 1.5F, -1.0F };
    glm::quat const rotation = glm::quat_identity<float, glm::defaultp>();

    g_window_overlay->SetTransformWorldRelative(vr::TrackingUniverseStanding, position, rotation);
    g_window_overlay->Show();
}

static void create_dashboard_overlay()
{
    g_dashboard_overlay->Create(vr::VROverlayType_Dashboard, DASHBOARD_KEY, DASHBOARD_NAME);

    // when overlay is VROverlayType_Dashboard we should set a thumbnail for the dashboard
    std::string thumbnail_path = {};
    thumbnail_path += SDL_GetCurrentDirectory();
    thumbnail_path += "icon.png";
    g_dashboard_overlay->SetThumbnail(thumbnail_path);

    g_dashboard_overlay->SetInputMethod(vr::VROverlayInputMethod_Mouse);
    g_dashboard_overlay->SetWidth(2.5F);

    g_dashboard_overlay->EnableFlag(vr::VROverlayFlags_SendVRDiscreteScrollEvents);
    g_dashboard_overlay->EnableFlag(vr::VROverlayFlags_EnableClickStabilization);
}

/** Returns true if the application should quit */
static bool handle_vr_event(const VrOverlay* overlay)
{
    vr::VREvent_t vr_event = {};
    while (vr::VROverlay()->PollNextOverlayEvent(overlay->Handle(), &vr_event, sizeof(vr_event)))
    {
        ImGui_ImplOpenVR_ProcessOverlayEvent(vr_event);

        switch (vr_event.eventType)
        {
        case vr::VREvent_PropertyChanged:
        {
            // Some drivers such as lighthouse or vrlink are capable of changing
            // vr::Prop_DisplayFrequency_Float without restarting SteamVR
            if (vr_event.data.property.prop == vr::Prop_DisplayFrequency_Float)
            {
                UpdateApplicationRefreshRate();
            }
            break;
        }
        case vr::VREvent_Quit:
        {
            g_ticking = false;
            return true;
        }
        default:
            break;
        }
    }
    return false;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    // Initialize the overlay as "VRApplication_Background" instead of "VRApplication_Overlay"
    // This makes sure that the overlay *cannot* run while SteamVR is not running.
    try
    {
        OpenVRInit(vr::VRApplication_Background);
    }
    catch (std::exception& ex)
    {
        spdlog::error("Failed to initialize OpenVR: {}", ex.what());
        return EXIT_FAILURE;
    }

    UpdateApplicationRefreshRate();

    try
    {
        if (!OpenVRManifestInstalled(APP_KEY))
            OpenVRManifestInstall();
    }
    catch (std::exception& ex)
    {
        spdlog::error("Failed to install OpenVR manifest: {}", ex.what());
        return EXIT_FAILURE;
    }

    try
    {
        create_dashboard_overlay();
        create_window_overlay();
    }
    catch (std::exception& ex)
    {
        spdlog::error("Failed to create or setup overlay: {}", ex.what());
        return EXIT_FAILURE;
    }

    g_vulkanRenderer->Initialize();

    g_ImGuiOverlayWindow->Initialize(g_vulkanRenderer, g_window_overlay, WIN_WIDTH, WIN_HEIGHT);
    g_ImGuiOverlayWindow->Initialize(g_vulkanRenderer, g_dashboard_overlay, WIN_WIDTH, WIN_HEIGHT);

    while (g_ticking)
    {
        if (handle_vr_event(g_window_overlay))
        {
            return 0;
        }
        if (handle_vr_event(g_dashboard_overlay))
        {
            return 0;
        }

        g_ImGuiOverlayWindow->Draw();

        ImDrawData* draw_data = ImGui::GetDrawData();

        g_vulkanRenderer->RenderOverlay(draw_data, g_window_overlay);
        g_vulkanRenderer->RenderOverlay(draw_data, g_dashboard_overlay);

        const auto target_time = static_cast<uint64_t>((static_cast<float>(1000000000) / g_hmd_refresh_rate));
        // ReSharper disable once CppTooWideScopeInitStatement
        const uint64_t frame_duration = (SDL_GetTicksNS() - g_last_frame_time);

        if (frame_duration < target_time)
        {
            SDL_DelayPrecise(target_time - frame_duration);
        }

        g_last_frame_time = SDL_GetTicksNS();
    }

    const VkResult vk_result = vkDeviceWaitIdle(g_vulkanRenderer->Device());
    VK_VALIDATE_RESULT(vk_result);

    g_ImGuiOverlayWindow->Destroy();
    g_vulkanRenderer->DestroyWindow(g_imGuiWindow->WindowData());
    g_imGuiWindow->Destroy(g_vulkanRenderer);
    g_vulkanRenderer->Destroy();

    ImGui::DestroyContext();

    SDL_Quit();

    return 0;
}
