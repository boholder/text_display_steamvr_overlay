#include "Settings.h"

#include <imgui.h>
#include <optional>
#include <string>
#include <numeric>

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
    SPDLOG_INFO("Apply changed settings and save to [{}]", settings.config_file_path);

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

#define IMPL_HELPER_FUNC(K)                  \
    bool Settings::K##_changed()             \
    { return settings.K != last_applied.K; } \
    void Settings::revert_##K()              \
    { settings.K = last_applied.K; }

IMPL_HELPER_FUNC(subtitle_font_color);
IMPL_HELPER_FUNC(subtitle_font_size);
IMPL_HELPER_FUNC(show_boarder_around_subtitle);
IMPL_HELPER_FUNC(subtitle_frame_width);
IMPL_HELPER_FUNC(subtitle_frame_height);
IMPL_HELPER_FUNC(subtitle_background_color);
IMPL_HELPER_FUNC(tcp_server_port);

bool Settings::operator==(const Settings& other) const // NOLINT(*-overloaded-operator)
{
#define EQ(K) K == other.K

    return EQ(subtitle_font_size) && EQ(subtitle_font_color) && EQ(subtitle_background_color) && EQ(show_boarder_around_subtitle)
           && EQ(subtitle_frame_width) && EQ(subtitle_frame_height) && EQ(tcp_server_port);
}

void Settings::write_yaml_to(YAML::Emitter& o) const
{
#define K YAML::Key <<
#define V << YAML::Value <<
#define KV(key) K #key V key
#define KV_COLOR(key) K #key V std::format("#{:08X}", util::revert_color_channel_order(key))

    o << YAML::BeginMap;
    o << KV(subtitle_font_size);
    o << YAML::Comment("color channel sequence: RRGGBBAA, eight hex bits corresponding to the Red, Green, Blue, Alpha channel");
    o << KV_COLOR(subtitle_font_color);
    o << KV_COLOR(subtitle_background_color);
    o << KV(show_boarder_around_subtitle);
    o << KV(subtitle_frame_width);
    o << KV(subtitle_frame_height);
    o << KV(tcp_server_port);
    o << YAML::Newline;
    o << YAML::Comment("debug mode: enable more detailed logging, can only be changed in here, won't show in the dashboard");
    o << KV(debug_mode);
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

#define APPLY_COLOR(K)                                                     \
    if (f[#K])                                                             \
    {                                                                      \
        const auto str = f[#K].as<std::string>();                          \
        const uint32_t rgba = strtoul(str.substr(1).c_str(), nullptr, 16); \
        settings.K = util::revert_color_channel_order(rgba);               \
    }

    APPLY_COLOR(subtitle_font_color);
    APPLY(subtitle_font_size);
    APPLY(show_boarder_around_subtitle);
    APPLY(subtitle_frame_width);
    APPLY(subtitle_frame_height);
    APPLY_COLOR(subtitle_background_color);
    APPLY(tcp_server_port);
    APPLY(debug_mode);

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
