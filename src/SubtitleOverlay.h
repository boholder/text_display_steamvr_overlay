#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H

#include <algorithm>

#include "Settings.h"
#include "constants.h"
#include "imgui.h"
#include "base/ImGuiUtils.h"
#include "base/ImGuiWindow.h"
#include "backends/imgui_impl_openvr.h"

namespace subtitle
{

static VrOverlay* create_overlay()
{
    static auto* ovl = new VrOverlay();
    ovl->Create(vr::VROverlayType_World, SUBTITLE_KEY, SUBTITLE_NAME);

    ovl->SetInputMethod(vr::VROverlayInputMethod_Mouse);

    ovl->EnableFlag(vr::VROverlayFlags_SendVRDiscreteScrollEvents);
    ovl->EnableFlag(vr::VROverlayFlags_EnableClickStabilization);
    ovl->EnableFlag(vr::VROverlayFlags_MakeOverlaysInteractiveIfVisible);

#ifdef WINDOW_SET_DEVICE_RELATIVE
    ovl->SetWidth(0.15f);

    // Device relative offset
    glm::vec3 position = {-0.10, 0, 0.10};
    glm::quat rotation
        = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 1, 0)) * glm::angleAxis(-glm::half_pi<float>(), glm::vec3(1, 0, 0));
    rotation *= glm::angleAxis(glm::radians(10.0f), glm::vec3(0, 1, 0));
    rotation = glm::normalize(rotation);

    ovl->SetTransformDeviceRelative(vr::TrackedControllerRole_LeftHand, position, rotation);
#elif WINDOW_SET_WORLD_RELATIVE
    ovl->SetWidth(1.0f);

    // Origin relative offset
    glm::vec3 position = {0.0f, 1.5f, -1.0f};
    glm::quat rotation = glm::quat_identity<float, glm::defaultp>();

    ovl->SetTransformWorldRelative(vr::TrackingUniverseStanding, position, rotation);
#endif

    ovl->Show();
    return ovl;
}

static void draw()
{
    settings.apply_to_subtitle();

    const int window_flags = im_util::set_next_window_fill_os_window();

    ImGui::Begin(SUBTITLE_NAME, nullptr, window_flags | ImGuiWindowFlags_NoBackground);

    settings.subtitle_window_width = ImGui::GetContentRegionAvail().x;
    settings.subtitle_window_height = ImGui::GetContentRegionAvail().y;
    const ImVec2 subtitle_frame_size(settings.subtitle_frame_width, settings.subtitle_frame_height);

    // make subtitle posited in center of window
    float sub_frame_x_pos = (settings.subtitle_window_width - settings.subtitle_frame_width) * 0.5F;
    sub_frame_x_pos = std::max(sub_frame_x_pos, 0.0F);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + sub_frame_x_pos);

    int child_window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;

    if (settings.show_boarder_around_subtitle)
    {
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 0, 0, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 5.0F);
        // NoBackground flag will prevent boarder being shown, unset it
        child_window_flags = child_window_flags & ~ImGuiWindowFlags_NoBackground;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::BeginChild("text_boarder", subtitle_frame_size, ImGuiChildFlags_Borders, child_window_flags);

    // subtitle text uses its own specific font size and color
    ImGui::PushFont(nullptr, settings.subtitle_font_size);
    ImGui::PushStyleColor(ImGuiCol_Text, settings.get_subtitle_font_color());
    ImGui::TextWrapped(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
        "enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
        "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
        "sunt in culpa qui officia deserunt mollit anim id est laborum."
    );
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::EndChild();

    if (settings.show_boarder_around_subtitle)
    {
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
    }

    ImGui::PopStyleVar();

    im_util::show_im_window_debug_info();

    ImGui::End();
}

static ImGuiWindow* init_window(VulkanRenderer*& g_vulkanRenderer, float g_dpiScale)
{
    const auto w = new ImGuiWindow(SUBTITLE_INDEX);
    w->Initialize(
        g_vulkanRenderer,
        SUBTITLE_NAME,
        SUBTITLE_WIDTH,
        SUBTITLE_HEIGHT,
        g_dpiScale,
        subtitle::draw,
        SDL_WINDOWPOS_CENTERED,
        20, // offset from top of monitor, enough to show a part of titlebar for cursor to drag
        SDL_WINDOW_TRANSPARENT
    );

    settings.apply_to_subtitle();

    g_vulkanRenderer->SetupOverlay(SUBTITLE_INDEX, SUBTITLE_WIDTH, SUBTITLE_HEIGHT, w->WindowData()->surface_format);

    return w;
}

static ImGuiOverlayWindow* init_ovl_window(VulkanRenderer*& g_vulkanRenderer, VrOverlay*& g_subtitle_overlay)
{
    const auto w = new ImGuiOverlayWindow();
    w->Initialize(g_vulkanRenderer, g_subtitle_overlay, SUBTITLE_WIDTH, SUBTITLE_HEIGHT, SUBTITLE_INDEX, subtitle::draw);
    return w;
}

} // namespace subtitle

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
