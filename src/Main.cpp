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

#include "backends/imgui_impl_openvr.h"

#include "constants.h"

#ifdef _WIN32
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
extern "C" __declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif

static VulkanRenderer* g_vulkanRenderer = new VulkanRenderer();
static ImGuiWindow* g_window_window = new ImGuiWindow();
static ImGuiWindow* g_dashboard_window = new ImGuiWindow();
static ImGuiOverlayWindow* g_ImGuiOverlayWindow = new ImGuiOverlayWindow();
static VrOverlay* g_overlay = new VrOverlay();

/**
 * current refresh rate of the headset. retrieved at every frame
 */
static float g_hmd_refresh_rate = 24.0f;
static float g_dpiScale = 0.0f;

#define WIN_WIDTH 1280
#define WIN_HEIGHT 720

static auto UpdateApplicationRefreshRate() -> void
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
        if (g_hmd_refresh_rate == 24.0f)
            std::exit(EXIT_FAILURE);
    }
}

static void create_dashboard_overlay()
{
    g_overlay->Create(vr::VROverlayType_Dashboard, DASHBOARD_KEY, DASHBOARD_NAME);

    // when overlay is VROverlayType_Dashboard we should set a thumbnail for the dashboard
    std::string thumbnail_path = {};
    thumbnail_path += SDL_GetCurrentDirectory();
    thumbnail_path += "icon.png";
    g_overlay->SetThumbnail(thumbnail_path);

    g_overlay->SetInputMethod(vr::VROverlayInputMethod_Mouse);
    g_overlay->SetWidth(2.5f);

    g_overlay->EnableFlag(vr::VROverlayFlags_SendVRDiscreteScrollEvents);
    g_overlay->EnableFlag(vr::VROverlayFlags_EnableClickStabilization);
}

static void create_window_overlay()
{
    g_overlay->Create(vr::VROverlayType_World, WINDOW_KEY, WINDOW_NAME);

    g_overlay->SetInputMethod(vr::VROverlayInputMethod_Mouse);

    g_overlay->EnableFlag(vr::VROverlayFlags_SendVRDiscreteScrollEvents);
    g_overlay->EnableFlag(vr::VROverlayFlags_EnableClickStabilization);
    g_overlay->EnableFlag(vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible);

#ifdef WINDOW_SET_DEVICE_RELATIVE
    g_overlay->SetWidth(0.15f);

    // Device relative offset
    glm::vec3 position = {-0.10, 0, 0.10};
    glm::quat rotation
        = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0));
    rotation *= glm::angleAxis(glm::radians(10.0f), glm::vec3(0, 1, 0));
    rotation = glm::normalize(rotation);

    g_overlay->SetTransformDeviceRelative(vr::TrackedControllerRole_LeftHand, position, rotation);
#elif WINDOW_SET_WORLD_RELATIVE
    g_overlay->SetWidth(1.0f);

    // Origin relative offset
    glm::vec3 position = {0.0f, 1.5f, -1.0f};
    glm::quat rotation = glm::quat_identity<float, glm::defaultp>();

    g_overlay->SetTransformWorldRelative(vr::TrackingUniverseStanding, position, rotation);
#endif

    g_overlay->Show();
}

/** @return whether the application should quit */
static bool handle_openvr_events(const VrOverlay* overlay, ImGuiWindow* window)
{
    static vr::VREvent_t vr_event = {};
    while (vr::VROverlay()->PollNextOverlayEvent(overlay->Handle(), &vr_event, sizeof(vr_event))) // NOLINT(*-unroll-loops)
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
#ifdef IMGUI_SDL_PLATFORM_BACKEND
        case vr::VREvent_OverlayShown:
        {
            if (overlay->IsVisible() && window->Shown())
            {
                window->Hide();
            }
            break;
        }
        case vr::VREvent_OverlayHidden:
        {
            if (!overlay->IsVisible() && window->Shown())
            {
                window->Show();
            }
            break;
        }
#endif
        case vr::VREvent_Quit:
        {
            return true;
        }
        default:
            break;
        }
    }
    return false;
}

/**
 * @return whether initialization was successful
 */
bool init_resources()
{
#ifdef NO_VR
    spdlog::info("Skipping OpenVR initialization");
#else
    // Initialize the overlay as "VRApplication_Background" instead of "VRApplication_Overlay"
    // This makes sure that the overlay *cannot* run while SteamVR is not running.
    try
    {
        OpenVRInit(vr::VRApplication_Background);
    }
    catch (std::exception& ex)
    {
        spdlog::error("Failed to initialize OpenVR: {}", ex.what());
        return false;
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
        return false;
    }

    try
    {
        create_dashboard_overlay();
        create_window_overlay();
    }
    catch (std::exception& ex)
    {
        spdlog::error("Failed to create or setup overlay: {}", ex.what());
        return false;
    }
#endif

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    auto sdl_init_flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
    if (!SDL_Init(sdl_init_flags))
    {
        spdlog::error("Failed to initialize SDL: {}", SDL_GetError());
        return false;
    }
#endif

    g_vulkanRenderer->Initialize();

#ifdef IMGUI_OPENVR_PLATFORM_BACKEND
    g_ImGuiOverlayWindow->Initialize(g_vulkanRenderer, g_overlay, WIN_WIDTH, WIN_HEIGHT);
#endif

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    g_dpiScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    g_window_window->Initialize(g_vulkanRenderer, WINDOW_NAME, WIN_WIDTH, WIN_HEIGHT, g_dpiScale);
    g_vulkanRenderer->SetupOverlay(0, WIN_WIDTH, WIN_HEIGHT, g_window_window->WindowData()->surface_format);
    g_dashboard_window->Initialize(g_vulkanRenderer, DASHBOARD_NAME, WIN_WIDTH, WIN_HEIGHT, g_dpiScale);
    g_vulkanRenderer->SetupOverlay(0, WIN_WIDTH, WIN_HEIGHT, g_dashboard_window->WindowData()->surface_format);
#endif

    return true;
}

void clean_resources()
{
    VkResult vk_result = vkDeviceWaitIdle(g_vulkanRenderer->Device());
    VK_VALIDATE_RESULT(vk_result);

    if (g_ImGuiOverlayWindow)
    {
        g_ImGuiOverlayWindow->Destroy();
    }
    if (g_window_window)
    {
        g_vulkanRenderer->DestroyWindow(g_window_window->WindowData());
        g_window_window->Destroy();
    }
    if (g_dashboard_window)
    {
        g_vulkanRenderer->DestroyWindow(g_dashboard_window->WindowData());
        g_dashboard_window->Destroy();
    }
    g_vulkanRenderer->Destroy();

    ImGui::DestroyContext();

    SDL_Quit();
}

static std::pair<int, int> update_window_size_and_swapchain(ImGuiWindow* window)
{
    int fb_width = {};
    int fb_height = {};
    SDL_GetWindowSize(window->Window(), &fb_width, &fb_height);

    fb_width *= static_cast<int>(g_dpiScale);
    fb_height *= static_cast<int>(g_dpiScale);

    if ((fb_width != 0 && fb_height != 0)
        && (g_vulkanRenderer->ShouldRebuildSwapchain() || window->WindowData()->width != fb_width
            || window->WindowData()->height != fb_height))
    {
        ImGui_ImplVulkan_SetMinImageCount(g_vulkanRenderer->MinimumConcurrentImageCount());

        window->WindowData()->width = fb_width;
        window->WindowData()->height = fb_height;

        g_vulkanRenderer->SetupSwapchain(window->WindowData(), fb_width, fb_height);
        window->WindowData()->frame_index = 0;
    }
    return {fb_width, fb_height};
}

bool main_loop()
{
    bool ticking = true;

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    static SDL_Event event = {};
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        auto process_event = [&](ImGuiWindow* window)
        {
            if (event.type == SDL_EVENT_WINDOW_MINIMIZED && window->IsMyEvent(&event))
                window->SetMinimizedFromEvent(true);
            if (event.type == SDL_EVENT_WINDOW_RESTORED && window->IsMyEvent(&event))
                window->SetMinimizedFromEvent(false);
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && window->IsMyEvent(&event))
                ticking = false;
        };

        process_event(g_window_window);
        process_event(g_dashboard_window);
    }
#endif

#ifndef NO_VR
    ticking = !handle_openvr_events(g_overlay, g_window_window);
    ticking = !handle_openvr_events(g_overlay, g_dashboard_window);
#endif

#ifdef IMGUI_OPENVR_PLATFORM_BACKEND
    g_ImGuiOverlayWindow->Draw();
#endif

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    {
        ImGuiIO& io = ImGui::GetIO();

        if (!io.WantTextInput)
        {
            g_window_window->SetKeyboardActiveState(false);
            g_dashboard_window->SetKeyboardActiveState(false);
        }

#    ifndef NO_VR
        if (g_overlay && g_overlay->IsVisible() && !g_window_window->KeyboardActive() && io.WantTextInput)
        {
            g_overlay->ShowKeyboard(vr::k_EGamepadTextInputModeNormal);
            g_window_window->SetKeyboardActiveState(true);
        }
        if (g_overlay && g_overlay->IsVisible() && !g_dashboard_window->KeyboardActive() && io.WantTextInput)
        {
            g_overlay->ShowKeyboard(vr::k_EGamepadTextInputModeNormal);
            g_dashboard_window->SetKeyboardActiveState(true);
        }
#    endif
    }

    auto [width, height] = update_window_size_and_swapchain(g_window_window);
    auto [width2, height2] = update_window_size_and_swapchain(g_dashboard_window);

#    ifndef NO_VR
    g_overlay->SetMouseScale(width, height);
    g_overlay->SetMouseScale(width2, height2);
#    endif
    g_window_window->Draw();
    g_dashboard_window->Draw();
#endif

    ImDrawData* draw_data = ImGui::GetDrawData();

#ifdef IMGUI_OPENVR_PLATFORM_BACKEND
#    ifndef NO_VR
    g_vulkanRenderer->RenderOverlay(0, draw_data, g_overlay);
#    endif
#endif

#ifdef IMGUI_SDL_PLATFORM_BACKEND
    g_window_window->WindowData()->set_background_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f));
    const bool is_minimized = g_window_window->Shown() && g_window_window->Minimized();
    g_window_window->WindowData()->is_minimized = is_minimized;

    if (!is_minimized)
    {
        g_vulkanRenderer->RenderWindow(draw_data, g_window_window->WindowData());
        g_vulkanRenderer->Present(g_window_window->WindowData());
    }

    g_dashboard_window->WindowData()->set_background_color(ImVec4(0.45f, 0.55f, 0.60f, 1.00f));
    const bool is_minimized2 = g_dashboard_window->Shown() && g_dashboard_window->Minimized();
    g_dashboard_window->WindowData()->is_minimized = is_minimized;

    if (!is_minimized2)
    {
        g_vulkanRenderer->RenderWindow(draw_data, g_dashboard_window->WindowData());
        g_vulkanRenderer->Present(g_dashboard_window->WindowData());
    }

#    ifndef NO_VR
    g_vulkanRenderer->RenderOverlay(0, draw_data, g_overlay);
#    endif
#endif

    // If we rendered this frame faster than the headset needs, pause a little
    static uint64_t last_frame_time = SDL_GetTicksNS();
    const uint64_t device_frame_duration = static_cast<float>(1'000'000'000) / g_hmd_refresh_rate;
    const uint64_t frame_duration = SDL_GetTicksNS() - last_frame_time;
    if (frame_duration < device_frame_duration)
    {
        SDL_DelayPrecise(device_frame_duration - frame_duration);
    }

    last_frame_time = SDL_GetTicksNS();

    return ticking;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    if (!init_resources())
        return EXIT_FAILURE;
    spdlog::info("Successfully initialized");

    bool ticking = true;
    while (ticking)
    {
        ticking = main_loop();
    }

    spdlog::info("Quit event received, shutting down...");
    clean_resources();
    return EXIT_SUCCESS;
}
