#include "Settings.h"

#include <imgui.h>

bool Settings::dirty = true;
bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;

Settings::Settings() {}

auto settings = Settings();

static void apply_to_imgui();

void Settings::apply_to_subtitle()
{
    if (dirty_to_subtitle)
    {
        dirty_to_subtitle = false;
        apply_to_imgui();
    }
}

void Settings::apply_to_dashboard()
{
    if (dirty_to_dashboard)
    {
        dirty_to_dashboard = false;
        apply_to_imgui();
    }
}

static void apply_to_imgui()
{
    const ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("NotoSans-Regular.ttf");
}
