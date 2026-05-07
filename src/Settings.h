#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <imgui.h>
#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

#define SUBTITLE_FONT_SIZE_MIN 10
#define SUBTITLE_FONT_SIZE_DEFAULT 20
#define SUBTITLE_FONT_SIZE_MAX 50

#define SUBTITLE_FONT_COLOR_DEFAULT {1.0F, 1.0F, 1.0F, 1.0F}

#define SUBTITLE_FRAME_WIDTH_MIN 500
#define SUBTITLE_FRAME_WIDTH_DEFAULT 1280
#define SUBTITLE_FRAME_WIDTH_MAX 4000

#define SUBTITLE_FRAME_HEIGHT_MIN 200
#define SUBTITLE_FRAME_HEIGHT_DEFAULT 500
#define SUBTITLE_FRAME_HEIGHT_MAX 2000

#define TCP_SERVER_DEFAULT_PORT 18781

class Settings
{
public:
    /**
     * Options that remain in effect after being set, only apply once per changed.
     */
    static void apply_to_subtitle();
    /**
     * Similar to apply_to_subtitle
     */
    static void apply_to_dashboard();

    /**
     * Save current settings to last_applied
     */
    static void apply_current();
    /**
     * Revert settings to last_applied
     */
    static void revert_to_last_applied();
    static bool has_changed_after_last_applied();

    float subtitle_font_color[4] = SUBTITLE_FONT_COLOR_DEFAULT;
    [[nodiscard]] ImU32 get_subtitle_font_color() const;

    int subtitle_font_size = SUBTITLE_FONT_SIZE_DEFAULT;
    bool show_boarder_around_subtitle = false;
    int subtitle_frame_width = SUBTITLE_FRAME_WIDTH_DEFAULT;
    int subtitle_frame_height = SUBTITLE_FRAME_HEIGHT_DEFAULT;

    int tcp_server_port = TCP_SERVER_DEFAULT_PORT;
    [[nodiscard]] std::optional<std::string> validate_tcp_server_port() const;
    static bool is_tcp_server_port_changed();

private:
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;

    /**
     * A backup that saves settings that
     * the last time the user presses the "Apply" button
     */
    static Settings last_applied;

    bool operator==(const Settings& other) const;

    static YAML::Node to_yaml(const Settings& s);
    static std::string generate_config_comment();
};

/**
 * Currently working settings,
 * other logic will retrieve option values from it,
 * dynamically changed by the user via Dashboard.
 */
extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
