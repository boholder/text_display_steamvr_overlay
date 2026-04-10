#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H

#include "constants.h"
#include "imgui.h"
#include "ImGuiWindow.h"

namespace dashboard
{

static void draw()
{
    ImGuiIO const& io = ImGui::GetIO();
    ImGui::Begin(DASHBOARD_NAME);
    ImGui::Text("D");
    ImGui::Text("Current context: %p", static_cast<void*>(ImGui::GetCurrentContext()));
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0F / io.Framerate, io.Framerate);
    ImGui::End();

    {
        static bool show_demo = true;
        ImGui::ShowDemoWindow(&show_demo);
    }
}

static ImGuiWindow* init(VulkanRenderer*& g_vulkanRenderer, float g_dpiScale)
{
    const auto w = new ImGuiWindow();
    w->Initialize(g_vulkanRenderer, DASHBOARD_NAME, DASHBOARD_WIDTH, DASHBOARD_HEIGHT, g_dpiScale, dashboard::draw);
    return w;
}

} // namespace dashboard

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
