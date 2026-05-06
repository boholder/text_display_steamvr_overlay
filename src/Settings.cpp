#include "Settings.h"

#include <imgui.h>
#include <optional>
#include <string>
#include "log.h"
#include "utils.h"
#include "constants.h"

bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;
float Settings::subtitle_font_color[4] = SUBTITLE_FONT_COLOR_DEFAULT;
int Settings::subtitle_font_size = SUBTITLE_FONT_SIZE_DEFAULT;
bool Settings::show_boarder_around_subtitle = false;
int Settings::subtitle_frame_width = SUBTITLE_FRAME_WIDTH_DEFAULT;
int Settings::subtitle_frame_height = SUBTITLE_FRAME_HEIGHT_DEFAULT;
int Settings::tcp_server_port = TCP_SERVER_DEFAULT_PORT;

Settings Settings::last_applied = clone();

auto settings = Settings();

void Settings::apply_current()
{
    SPDLOG_INFO("Apply changed settings");

    std::stringstream ss;
    ss << to_yaml(last_applied);
    const std::string previous = ss.str();

    last_applied = clone();

    std::stringstream ss2;
    ss2 << to_yaml(last_applied);
    const std::string current = ss2.str();

    SPDLOG_DEBUG("Setting changing details:\n{}", util::diff_lines(previous, current));

    util::save_to_file("settings.yaml", generate_config_comment() + current);
}

/**
 * @return some words about the configuration file, for users who surprisingly find it on their disk and wondering what the file is.
 */
std::string Settings::generate_config_comment()
{
    std::stringstream time;
    auto t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    time << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return std::format("# This is a configuration file generated and used by [{} v{}] at {}.\n", APP_NAME, APP_VERSION, time.str())
           + "# You can manually change its content while the application is NOT running, or it might be overridden by the application.\n"
           + std::format("# Check the link for more information: {}\n", APP_LINK);
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
    tcp_server_port = last_applied._tcp_server_port;
}

bool Settings::has_changed()
{
    const bool text_color = subtitle_font_color[0] != last_applied._subtitle_font_color[0]
                            || subtitle_font_color[1] != last_applied._subtitle_font_color[1]
                            || subtitle_font_color[2] != last_applied._subtitle_font_color[2]
                            || subtitle_font_color[3] != last_applied._subtitle_font_color[3];
    const bool text_size = subtitle_font_size != last_applied._subtitle_font_size
                           || show_boarder_around_subtitle != last_applied._show_boarder_around_subtitle
                           || subtitle_frame_width != last_applied._subtitle_frame_width
                           || subtitle_frame_height != last_applied._subtitle_frame_height;
    const bool tcp_port = tcp_server_port != last_applied._tcp_server_port;
    return text_color || text_size || tcp_port;
}

YAML::Node Settings::to_yaml(const Settings& s)
{
    YAML::Node node;
    const ImU32 c = ImGui::ColorConvertFloat4ToU32(
        ImVec4(s._subtitle_font_color[0], s._subtitle_font_color[1], s._subtitle_font_color[2], s._subtitle_font_color[3]));
    node["subtitle_font_color"] = std::format("#{:08X}", c);
    node["subtitle_font_size"] = s._subtitle_font_size;
    node["show_boarder_around_subtitle"] = s._show_boarder_around_subtitle;
    node["subtitle_frame_width"] = s._subtitle_frame_width;
    node["subtitle_frame_height"] = s._subtitle_frame_height;
    node["tcp_server_port"] = s._tcp_server_port;
    return node;
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
