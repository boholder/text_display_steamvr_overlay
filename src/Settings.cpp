#include "Settings.h"

#include <imgui.h>

bool Settings::dirty = true;
bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;
float Settings::subtitle_font_color[3] = {1.0F, 1.0F, 1.0F};

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

ImVec4 Settings::get_subtitle_font_color()
{ return {subtitle_font_color[0], subtitle_font_color[1], subtitle_font_color[2], 1.0F}; }
