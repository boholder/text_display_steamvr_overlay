#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H

#include "constants.h"
#include "imgui.h"
#include "ImGuiWindow.h"

namespace subtitle
{

static void draw()
{
    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    bool open_ptr = true;

    ImGuiIO const& io = ImGui::GetIO();
    ImGui::Begin(SUBTITLE_NAME, &open_ptr, window_flags);
    ImGui::Text("W");
    ImGui::Text("Current context: %p", static_cast<void*>(ImGui::GetCurrentContext()));
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0F / io.Framerate, io.Framerate);
    ImGui::End();
}

static ImGuiWindow* init(VulkanRenderer*& g_vulkanRenderer, float g_dpiScale)
{
    const auto w = new ImGuiWindow();
    w->Initialize(
        g_vulkanRenderer,
        SUBTITLE_NAME,
        SUBTITLE_WIDTH,
        SUBTITLE_HEIGHT,
        g_dpiScale,
        subtitle::draw,
        SDL_WINDOW_TRANSPARENT | SDL_WINDOW_BORDERLESS
    );
    return w;
}

} // namespace subtitle

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
