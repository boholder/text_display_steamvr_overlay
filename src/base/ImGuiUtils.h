#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_IMGUIUTILS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_IMGUIUTILS_H
#include "ImGuiWindow.h"

namespace im_util
{

/**
 * ref: https://github.com/ocornut/imgui/issues/3541#issuecomment-712248014
 *
 * @return window flags that are used in ImGui::Begin()
 */
static int set_next_window_fill_os_window()
{
#ifdef IMGUI_HAS_VIEWPORT
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
#else
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

    return ImGuiWindowFlags_NoDecoration;
}

static void show_im_window_debug_info()
{
#ifdef ENABLE_DEBUG_UI
    ImGui::SeparatorText("Debug Info");
    ImGui::Text("Current context: %p", static_cast<void*>(ImGui::GetCurrentContext()));
    ImGuiIO const& io = ImGui::GetIO();
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0F / io.Framerate, io.Framerate);
#endif
}

} // namespace im_util

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_IMGUIUTILS_H
