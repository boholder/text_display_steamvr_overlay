#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H

#include "constants.h"
#include "imgui.h"
#include "ImGuiWindow.h"

namespace dashboard
{

static VrOverlay* create_overlay()
{
    static auto *ovl = new VrOverlay();
    ovl->Create(vr::VROverlayType_Dashboard, DASHBOARD_KEY, DASHBOARD_NAME);

    // when overlay is VROverlayType_Dashboard we should set a thumbnail for the dashboard
    std::string thumbnail_path = {};
    thumbnail_path += SDL_GetCurrentDirectory();
    thumbnail_path += "icon.png";
    ovl->SetThumbnail(thumbnail_path);

    ovl->SetInputMethod(vr::VROverlayInputMethod_Mouse);
    ovl->SetWidth(2.5F);

    ovl->EnableFlag(vr::VROverlayFlags_SendVRDiscreteScrollEvents);
    ovl->EnableFlag(vr::VROverlayFlags_EnableClickStabilization);
    return ovl;
}

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
    auto *const w = new ImGuiWindow();
    w->Initialize(g_vulkanRenderer, DASHBOARD_NAME, DASHBOARD_WIDTH, DASHBOARD_HEIGHT, g_dpiScale, dashboard::draw);
    return w;
}

} // namespace dashboard

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
