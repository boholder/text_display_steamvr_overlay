#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H

#include "Settings.h"
#include "constants.h"
#include "imgui.h"
#include "base/ImGuiUtils.h"
#include "base/ImGuiWindow.h"

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

    ImGui::Begin(SUBTITLE_NAME, nullptr, window_flags | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar);

    ImGui::PushFont(nullptr, settings.subtitle_font_size);
    ImGui::PushStyleColor(ImGuiCol_Text, settings.get_subtitle_font_color()); // Red color
    ImGui::TextWrapped(
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut "
        "enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in "
        "reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, "
        "sunt in culpa qui officia deserunt mollit anim id est laborum."
    );
    ImGui::PopStyleColor();
    ImGui::PopFont();

    im_util::show_debug_info();

    ImGui::End();
}

static ImGuiWindow* init_window(VulkanRenderer*& g_vulkanRenderer, float g_dpiScale)
{
    const auto w = new ImGuiWindow();
    w->Initialize(
        g_vulkanRenderer,
        SUBTITLE_NAME,
        SUBTITLE_WIDTH,
        SUBTITLE_HEIGHT,
        g_dpiScale,
        subtitle::draw,
        SDL_WINDOWPOS_CENTERED,
        20,
        SDL_WINDOW_TRANSPARENT
    );
    settings.apply_to_subtitle();
    return w;
}

static ImGuiOverlayWindow* init_ovl_window(VulkanRenderer*& g_vulkanRenderer, VrOverlay*& g_subtitle_overlay)
{
    const auto w = new ImGuiOverlayWindow();
    w->Initialize(g_vulkanRenderer, g_subtitle_overlay, SUBTITLE_WIDTH, SUBTITLE_HEIGHT, 0, subtitle::draw);
    return w;
}

} // namespace subtitle

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
