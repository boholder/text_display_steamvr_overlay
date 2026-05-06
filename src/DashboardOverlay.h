#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H

#include <imgui.h>
#include "Settings.h"
#include "constants.h"
#include "base/ImGuiWindow.h"

static void validate_with_red_border(std::optional<std::string> (*validator)(), void (*draw_widget)())
{
    const std::optional<std::string> v = validator();

    if (v.has_value())
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0F);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(1.0F, 0.2F, 0.2F, 1.0F));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.30F, 0.10F, 0.10F, 1.0F));
    }

    draw_widget();

    if (v.has_value())
    {
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar();
        ImGui::TextColored(ImVec4(1.0F, 0.4F, 0.4F, 1.0F), "%s", v.value().c_str());
    }
}

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
    ImGui::SliderInt("Subtitle Font Size", &settings.subtitle_font_size, SUBTITLE_FONT_SIZE_MIN, SUBTITLE_FONT_SIZE_MAX);
    ImGui::ColorEdit4("Subtitle Font Color", settings.subtitle_font_color, ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox("Subtitle Boarder", &settings.show_boarder_around_subtitle);
    ImGui::SliderInt("Subtitle Frame Width", &settings.subtitle_frame_width, SUBTITLE_FRAME_WIDTH_MIN, SUBTITLE_FRAME_WIDTH_MAX);
    ImGui::SliderInt("Subtitle Frame Height", &settings.subtitle_frame_height, SUBTITLE_FRAME_HEIGHT_MIN, SUBTITLE_FRAME_HEIGHT_MAX);

    ImGui::SeparatorText("TCP Server Options");

    validate_with_red_border(settings.validate_tcp_server_port, [] { ImGui::InputInt("Port", &settings.tcp_server_port, 0, 0, 0); });

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::BeginDisabled();
    ImGui::Button("OK");
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool settings_not_changed = !Settings::has_changed();
    if (settings_not_changed)
        ImGui::BeginDisabled();
    if (ImGui::Button("Cancel"))
        Settings::revert_to_last_applied();
    ImGui::SameLine();
    if (ImGui::Button("Apply"))
        Settings::apply_current();
    if (settings_not_changed)
        ImGui::EndDisabled();

    im_util::show_im_window_debug_info();

    ImGui::End();

    // {
    //     static bool show_demo = true;
    //     ImGui::ShowDemoWindow(&show_demo);
    // }
}

static ImGuiWindow* init_window(VulkanRenderer*& g_vulkanRenderer, float g_dpiScale)
{
    auto* const w = new ImGuiWindow(DASHBOARD_INDEX);
    w->Initialize(g_vulkanRenderer,
                  DASHBOARD_NAME,
                  DASHBOARD_WIDTH,
                  DASHBOARD_HEIGHT,
                  g_dpiScale,
                  dashboard::draw,
                  SDL_WINDOWPOS_CENTERED,
                  250 * g_dpiScale);

    settings.apply_to_dashboard();

    g_vulkanRenderer->SetupOverlay(DASHBOARD_INDEX, DASHBOARD_WIDTH, DASHBOARD_HEIGHT, w->WindowData()->surface_format);

    return w;
}

static ImGuiOverlayWindow* init_ovl_window(VulkanRenderer*& g_vulkanRenderer, VrOverlay*& g_dashboard_overlay)
{
    const auto w = new ImGuiOverlayWindow();
    w->Initialize(g_vulkanRenderer, g_dashboard_overlay, DASHBOARD_WIDTH, DASHBOARD_HEIGHT, DASHBOARD_INDEX, dashboard::draw);
    return w;
}

} // namespace dashboard

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_DASHBOARDOVERLAY_H
