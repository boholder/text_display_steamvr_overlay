#include "Settings.h"

#include <imgui.h>
#include <optional>
#include <string>
#include "log.h"
#include "utils.h"
#include "constants.h"

bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;
Settings Settings::last_applied = Settings();

auto settings = Settings();

void Settings::apply_current()
{
    SPDLOG_INFO("Apply changed settings");

    std::stringstream ss;
    ss << to_yaml(last_applied);
    const std::string previous = ss.str();
    std::stringstream ss2;
    ss2 << to_yaml(settings);
    const std::string current = ss2.str();
    SPDLOG_DEBUG("Setting changing details:\n{}", util::diff_lines(previous, current));

    last_applied = settings;

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

void Settings::revert_to_last_applied()
{
    SPDLOG_INFO("Revert to last applied settings");
    settings = last_applied;
}

bool Settings::has_changed_after_last_applied()
{ return settings != last_applied; }

bool Settings::operator==(const Settings& other) const // NOLINT(*-overloaded-operator)
{
    return subtitle_font_color[0] == other.subtitle_font_color[0] && subtitle_font_color[1] == other.subtitle_font_color[1]
           && subtitle_font_color[2] == other.subtitle_font_color[2] && subtitle_font_color[3] == other.subtitle_font_color[3]
           && subtitle_font_size == other.subtitle_font_size && show_boarder_around_subtitle == other.show_boarder_around_subtitle
           && subtitle_frame_width == other.subtitle_frame_width && subtitle_frame_height == other.subtitle_frame_height
           && tcp_server_port == other.tcp_server_port;
}

YAML::Node Settings::to_yaml(const Settings& s)
{
    YAML::Node node;
    node["subtitle_font_color"] = std::format("#{:08X}", s.get_subtitle_font_color());
    node["subtitle_font_size"] = s.subtitle_font_size;
    node["show_boarder_around_subtitle"] = s.show_boarder_around_subtitle;
    node["subtitle_frame_width"] = s.subtitle_frame_width;
    node["subtitle_frame_height"] = s.subtitle_frame_height;
    node["tcp_server_port"] = s.tcp_server_port;
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

ImU32 Settings::get_subtitle_font_color() const
{
    return ImGui::ColorConvertFloat4ToU32(
        ImVec4(subtitle_font_color[0], subtitle_font_color[1], subtitle_font_color[2], subtitle_font_color[3]));
}

std::optional<std::string> Settings::validate_tcp_server_port() const
{
    if (tcp_server_port <= 1024 || tcp_server_port > 65535)
    {
        return std::make_optional<std::string>("must be within 1025~65535");
    }
    return std::nullopt;
}

bool Settings::is_tcp_server_port_changed()
{ return settings.tcp_server_port != last_applied.tcp_server_port; }
