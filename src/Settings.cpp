#include "Settings.h"

#include <imgui.h>

bool Settings::dirty = true;
bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;
float Settings::subtitle_font_color[4] = SUBTITLE_FONT_COLOR_DEFAULT;
float Settings::subtitle_font_size = SUBTITLE_FONT_SIZE_DEFAULT;
bool Settings::show_boarder_around_subtitle = false;
float Settings::subtitle_frame_width = SUBTITLE_FRAME_WIDTH_DEFAULT;
float Settings::subtitle_frame_height = SUBTITLE_FRAME_HEIGHT_DEFAULT;
float Settings::subtitle_window_width = SUBTITLE_FRAME_WIDTH_MAX;
float Settings::subtitle_window_height = SUBTITLE_FRAME_HEIGHT_MAX;

Settings::Settings() {}

auto settings = Settings();

static void apply_to_imgui_window();

void Settings::apply_to_subtitle()
{
    if (dirty_to_subtitle)
    {
        dirty_to_subtitle = false;
        apply_to_imgui_window();
    }
}

void Settings::apply_to_dashboard()
{
    if (dirty_to_dashboard)
    {
        dirty_to_dashboard = false;
        apply_to_imgui_window();
    }
}

static void apply_to_imgui_window()
{
    const ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("NotoSans-Regular.ttf");
}

ImU32 Settings::get_subtitle_font_color()
{
    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(subtitle_font_color[0], subtitle_font_color[1], subtitle_font_color[2], subtitle_font_color[3])
    );
}
