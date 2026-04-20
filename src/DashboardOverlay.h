#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H

#include "Settings.h"
#include "constants.h"
#include "imgui.h"
#include "base/ImGuiWindow.h"

namespace dashboard
{

static VrOverlay* create_overlay()
{
    static auto* ovl = new VrOverlay();
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
    settings.apply_to_dashboard();

    const int window_flags = im_util::set_next_window_fill_os_window();

    ImGui::Begin(DASHBOARD_NAME, nullptr, window_flags);

    ImGui::SeparatorText("Subtitle Options");
    ImGui::SliderFloat("Subtitle Font Size", &settings.subtitle_font_size, SUBTITLE_FONT_SIZE_MIN, SUBTITLE_FONT_SIZE_MAX);
    ImGui::ColorEdit4("Subtitle Font Color", settings.subtitle_font_color, ImGuiColorEditFlags_AlphaBar);

    im_util::show_im_window_debug_info();

    ImGui::End();

    // {
    //     static bool show_demo = true;
    //     ImGui::ShowDemoWindow(&show_demo);
    // }
}

static ImGuiWindow* init_window(VulkanRenderer*& g_vulkanRenderer, float g_dpiScale)
{
    auto* const w = new ImGuiWindow();
    w->Initialize(
        g_vulkanRenderer,
        DASHBOARD_NAME,
        DASHBOARD_WIDTH,
        DASHBOARD_HEIGHT,
        g_dpiScale,
        dashboard::draw,
        SDL_WINDOWPOS_CENTERED,
        SUBTITLE_HEIGHT * g_dpiScale
    );
    settings.apply_to_dashboard();
    return w;
}

static ImGuiOverlayWindow* init_ovl_window(VulkanRenderer*& g_vulkanRenderer, VrOverlay*& g_dashboard_overlay)
{
    const auto w = new ImGuiOverlayWindow();
    w->Initialize(g_vulkanRenderer, g_dashboard_overlay, DASHBOARD_WIDTH, DASHBOARD_HEIGHT, 1, dashboard::draw);
    return w;
}

} // namespace dashboard

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
