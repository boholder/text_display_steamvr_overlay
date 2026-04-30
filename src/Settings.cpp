#include "Settings.h"

#include <imgui.h>
#include <optional>
#include <string>
#include <spdlog/spdlog.h>

bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;
float Settings::subtitle_font_color[4] = SUBTITLE_FONT_COLOR_DEFAULT;
float Settings::subtitle_font_size = SUBTITLE_FONT_SIZE_DEFAULT;
bool Settings::show_boarder_around_subtitle = false;
float Settings::subtitle_frame_width = SUBTITLE_FRAME_WIDTH_DEFAULT;
float Settings::subtitle_frame_height = SUBTITLE_FRAME_HEIGHT_DEFAULT;
float Settings::subtitle_window_width = SUBTITLE_FRAME_WIDTH_MAX;
float Settings::subtitle_window_height = SUBTITLE_FRAME_HEIGHT_MAX;
int Settings::tcp_server_port = TCP_SERVER_DEFAULT_PORT;

Settings Settings::last_applied = clone();

auto settings = Settings();

void Settings::apply_current()
{
    SPDLOG_INFO("Apply changed settings");
    last_applied = clone();
}

Settings Settings::clone()
{
    Settings b;
    b._subtitle_font_color[0] = subtitle_font_color[0];
    b._subtitle_font_color[1] = subtitle_font_color[1];
    b._subtitle_font_color[2] = subtitle_font_color[2];
    b._subtitle_font_color[3] = subtitle_font_color[3];
    b._subtitle_font_size = subtitle_font_size;
    b._show_boarder_around_subtitle = show_boarder_around_subtitle;
    b._subtitle_frame_width = subtitle_frame_width;
    b._subtitle_frame_height = subtitle_frame_height;
    b._subtitle_window_width = subtitle_window_width;
    b._subtitle_window_height = subtitle_window_height;
    b._tcp_server_port = tcp_server_port;
    return b;
}

void Settings::revert_to_last_applied()
{
    SPDLOG_INFO("Revert to last applied settings");
    subtitle_font_color[0] = last_applied._subtitle_font_color[0];
    subtitle_font_color[1] = last_applied._subtitle_font_color[1];
    subtitle_font_color[2] = last_applied._subtitle_font_color[2];
    subtitle_font_color[3] = last_applied._subtitle_font_color[3];
    subtitle_font_size = last_applied._subtitle_font_size;
    show_boarder_around_subtitle = last_applied._show_boarder_around_subtitle;
    subtitle_frame_width = last_applied._subtitle_frame_width;
    subtitle_frame_height = last_applied._subtitle_frame_height;
    subtitle_window_width = last_applied._subtitle_window_width;
    subtitle_window_height = last_applied._subtitle_window_height;
    tcp_server_port = last_applied._tcp_server_port;
}

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
        ImVec4(subtitle_font_color[0], subtitle_font_color[1], subtitle_font_color[2], subtitle_font_color[3]));
}

std::optional<std::string> Settings::validate_tcp_server_port()
{
    if (tcp_server_port <= 1024 || tcp_server_port > 65535)
    {
        return std::make_optional<std::string>("must be within 1025~65535");
    }
    return std::nullopt;
}

bool Settings::is_tcp_server_port_changed()
{ return tcp_server_port != last_applied._tcp_server_port; }
