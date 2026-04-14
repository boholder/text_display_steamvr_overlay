#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLEOVERLAY_H

#include "Settings.h"
#include "constants.h"
#include "imgui.h"
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

    ImGuiWindowFlags window_flags = 0;
    window_flags |= ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize;
    bool open_ptr = true;

    ImGuiIO const& io = ImGui::GetIO();
    ImGui::Begin(SUBTITLE_NAME, &open_ptr, window_flags);
#ifdef ENABLE_DEBUG_UI
    ImGui::Text("Current context: %p", static_cast<void*>(ImGui::GetCurrentContext()));
    ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0F / io.Framerate, io.Framerate);
#endif
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
        0,
        SDL_WINDOW_TRANSPARENT | SDL_WINDOW_BORDERLESS
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
