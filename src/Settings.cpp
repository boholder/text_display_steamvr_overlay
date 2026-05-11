#include "Settings.h"

#include <imgui.h>
#include <optional>
#include <string>
#include "log.h"
#include "utils.h"
#include "constants.h"

#include <filesystem>

bool Settings::dirty_to_subtitle = true;
bool Settings::dirty_to_dashboard = true;
Settings Settings::last_applied = Settings();

auto settings = Settings();

void Settings::apply_current()
{
    SPDLOG_INFO("Apply changed settings");

    YAML::Emitter ss;
    last_applied.write_yaml_to(ss);
    const std::string previous = ss.c_str();
    YAML::Emitter ss2;
    settings.write_yaml_to(ss2);
    const std::string current = ss2.c_str();
    SPDLOG_DEBUG("Setting changing details:\n{}", util::diff_lines(previous, current));

    last_applied = settings;

    std::stringstream time;
    auto t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    time << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // some words about the configuration file, for users who surprisingly find it on their disk and wondering what the file it is.
    auto header
        = std::format("# This is a configuration file generated and used by [{} v{}] at {}.\n", APP_NAME, APP_VERSION, time.str())
          + "# You can manually change its content while the application is NOT running, or it might be overridden by the application.\n"
          + std::format("# Check the link for more information: {}\n", APP_LINK)
          + "# For yaml file formatting, see: https://yaml.org/spec/1.2/spec.html\n\n";

    util::save_to_file(settings.config_file_path, header + current);
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
    return subtitle_font_color == other.subtitle_font_color && subtitle_font_size == other.subtitle_font_size
           && show_boarder_around_subtitle == other.show_boarder_around_subtitle && subtitle_frame_width == other.subtitle_frame_width
           && subtitle_frame_height == other.subtitle_frame_height && tcp_server_port == other.tcp_server_port;
}

void Settings::write_yaml_to(YAML::Emitter& o) const
{
#define K YAML::Key <<
#define V << YAML::Value <<

    o << YAML::BeginMap;
    o << YAML::Comment("AARRGGBB: eight hex bits corresponding to the Alpha, Red, Green, Blue channel");
    o << K "subtitle_font_color" V std::format("#{:08X}", settings.subtitle_font_color);
    o << K "subtitle_font_size" V subtitle_font_size;
    o << K "show_boarder_around_subtitle" V show_boarder_around_subtitle;
    o << K "subtitle_frame_width" V subtitle_frame_width;
    o << K "subtitle_frame_height" V subtitle_frame_height;
    o << K "tcp_server_port" V tcp_server_port;
    o << YAML::EndMap;
}

void Settings::load_from_yaml_file(const std::string& config_file_path)
{
    settings.config_file_path = config_file_path;
    YAML::Node f = YAML::Load("");
    const bool config_file_exists = std::filesystem::exists(config_file_path);
    if (config_file_exists)
    {
        SPDLOG_INFO("Load settings from file [{}]", config_file_path);
        f = YAML::LoadFile(config_file_path);
    }

#define APPLY(K)                                  \
    if (f[#K])                                    \
    settings.K = f[#K].as<decltype(settings.K)>()
    // else 'settings' instance would use hardcoded default option values, as assigned when being inited.

    if (f["subtitle_font_color"])
    {
        const auto str = f["subtitle_font_color"].as<std::string>();
        settings.subtitle_font_color = strtoul(str.substr(1).c_str(), nullptr, 16);
    }

    APPLY(subtitle_font_size);
    APPLY(show_boarder_around_subtitle);
    APPLY(subtitle_frame_width);
    APPLY(subtitle_frame_height);
    APPLY(tcp_server_port);
    APPLY(config_file_path);

    if (!config_file_exists)
    {
        SPDLOG_INFO("Dump default settings to file [{}]", config_file_path);
    }
    apply_current();
}

static void apply_to_imgui_window()
{
    const ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("NotoSans-Regular.ttf");
}

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
