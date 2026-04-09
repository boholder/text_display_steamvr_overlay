/*
 * Copyright (C) 2025. Nyabsi <nyabsi@sovellus.cc>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "ImGuiWindow.h"

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>

#include "backends/imgui_impl_openvr.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <math.h>
#include <functional>

ImGuiWindow::ImGuiWindow()
{
    window_ = nullptr;
    window_data_ = {};
    window_shown_ = false;
    window_minimized_ = false;
    keyboard_active_ = false;
    draw_callback_ = nullptr;
}

auto ImGuiWindow::Initialize(VulkanRenderer*& renderer, const char* name, int width, int height, float dpiScale, void (*draw_callback)(), bool show) -> void
{
    this->draw_callback_ = draw_callback;

    auto sdl_window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN | SDL_WINDOW_MOUSE_FOCUS | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window_ = SDL_CreateWindow(name, width * static_cast<int>(dpiScale), height * static_cast<int>(dpiScale), sdl_window_flags);
    if (window_ == nullptr)
    {
        spdlog::error("[SDL] Failed to create a window: {}", SDL_GetError());
        return;
    }

    VkSurfaceKHR surface = {};
    if (SDL_Vulkan_CreateSurface(window_, renderer->Instance(), renderer->Allocator(), &surface) == 0)
    {
        spdlog::error("[SDL] Failed to create a Vulkan rendering surface for window: {}", SDL_GetError());
        return;
    }

    int initial_width = {};
    int initial_height = {};
    SDL_GetWindowSize(window_, &initial_width, &initial_height);

    VulkanWindow* window = &window_data_;
    renderer->SetupWindow(window, surface, initial_width, initial_height);

    SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_SetWindowFocusable(window_, show);
    SDL_CaptureMouse(false);

    window_shown_ = show;

    if (show)
    {
        SDL_ShowWindow(window_);
    }

    IMGUI_CHECKVERSION();

    imgui_context_ = ImGui::CreateContext();
    ImGui::SetCurrentContext(imgui_context_);
    ImGuiIO& io = ImGui::GetIO();
    ( void ) io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_IsSRGB; // NOTE: ImGuiConfigFlags_IsSRGB is not used by ImGui, used to communicate state.

    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.ScaleAllSizes(dpiScale);
    style.FontScaleDpi = dpiScale;

    if (io.ConfigFlags & ImGuiConfigFlags_IsSRGB)
    {
        // hack: ImGui doesn't handle sRGB colour spaces properly so convert from Linear -> sRGB
        // https://github.com/ocornut/imgui/issues/8271#issuecomment-2564954070
        // remove when these are merged:
        //  https://github.com/ocornut/imgui/pull/8110
        //  https://github.com/ocornut/imgui/pull/8111
        for (int i = 0; i < ImGuiCol_COUNT; i++)
        {
            ImVec4& col = style.Colors[i];
            col.x = col.x <= 0.04045f ? col.x / 12.92f : pow((col.x + 0.055f) / 1.055f, 2.4f);
            col.y = col.y <= 0.04045f ? col.y / 12.92f : pow((col.y + 0.055f) / 1.055f, 2.4f);
            col.z = col.z <= 0.04045f ? col.z / 12.92f : pow((col.z + 0.055f) / 1.055f, 2.4f);
        }
    }

    VkPipelineRenderingCreateInfoKHR pipeline_rendering_create_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .viewMask = 0,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &window->surface_format.format,
        .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };

    ImGui_ImplVulkan_InitInfo init_info = {
        .ApiVersion = VK_API_VERSION_1_3,
        .Instance = renderer->Instance(),
        .PhysicalDevice = renderer->PhysicalDevice(),
        .Device = renderer->Device(),
        .QueueFamily = renderer->QueueFamily(),
        .Queue = renderer->Queue(),
        .DescriptorPool = renderer->DescriptorPool(),
        .RenderPass = VK_NULL_HANDLE,
        .MinImageCount = renderer->MinimumConcurrentImageCount(),
        .ImageCount = window->image_count,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
        .PipelineCache = renderer->PipelineCache(),
        .Subpass = 0,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo = pipeline_rendering_create_info,
        .Allocator = renderer->Allocator(),
        .CheckVkResultFn = nullptr,
    };

    ImGui_ImplSDL3_InitForVulkan(window_);
    ImGui_ImplVulkan_Init(&init_info);
}

auto ImGuiWindow::Show() -> void
{
    SDL_ShowWindow(window_);
    SDL_MinimizeWindow(window_);

    window_shown_ = true;
}

auto ImGuiWindow::SetMinimizedFromEvent(bool state) -> void
{ window_minimized_ = state; }

auto ImGuiWindow::SetKeyboardActiveState(bool state) -> void
{ keyboard_active_ = state; }

auto ImGuiWindow::Hide() -> void
{
    SDL_RestoreWindow(window_);
    SDL_HideWindow(window_);

    window_shown_ = false;
}

auto ImGuiWindow::Draw() -> void
{
    ImGui::SetCurrentContext(imgui_context_);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    if (draw_callback_)
    {
        draw_callback_();
    }

    ImGui::Render();
}

auto ImGuiWindow::Destroy() -> void
{
    ImGui::SetCurrentContext(imgui_context_);
    ImGui_ImplSDL3_Shutdown();
    SDL_DestroyWindow(window_);
}

auto ImGuiWindow::IsMyEvent(const SDL_Event* event) const -> bool
{ return event->window.windowID == SDL_GetWindowID(window_); }

auto ImGuiWindow::DestroyContext() const -> void
{
    ImGui::SetCurrentContext(imgui_context_);
    ImGui::DestroyContext(imgui_context_);
}
