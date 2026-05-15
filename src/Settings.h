#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

#include "macro.h"

#define SUBTITLE_FONT_SIZE_MIN 10
#define SUBTITLE_FONT_SIZE_DEFAULT 20
#define SUBTITLE_FONT_SIZE_MAX 50

// color channel sequence: ABGR
// opaque white
#define SUBTITLE_FONT_COLOR_DEFAULT 0xFFFFFFFF

#define SUBTITLE_FRAME_WIDTH_MIN 500
#define SUBTITLE_FRAME_WIDTH_DEFAULT 1280
#define SUBTITLE_FRAME_WIDTH_MAX 4000

#define SUBTITLE_FRAME_HEIGHT_MIN 200
#define SUBTITLE_FRAME_HEIGHT_DEFAULT 500
#define SUBTITLE_FRAME_HEIGHT_MAX 2000

// when on VR, translucent black background
#define SUBTITLE_FRAME_BG_COLOR_DEFAULT 0x80000000
// when not, opaque black background
#define SUBTITLE_FRAME_BG_COLOR_DEFAULT_NO_VR 0xFF000000

#define TCP_SERVER_DEFAULT_PORT 18781

#define CONFIG_FILE_PATH "settings.yaml"
#define CONFIG_FILE_PATH_NO_VR "settings_debug.yaml"

#define DEBUG_MODE false

#define DEFINE_HELPER_FUNC(K)  \
    static bool K##_changed(); \
    static void revert_##K();

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
     * Save 'settings' to 'last_applied'
     * and update the content of the config file.
     */
    static void apply_current();
    /**
     * Revert settings to last_applied
     */
    static void revert_to_last_applied();
    static bool has_changed_after_last_applied();

    int subtitle_font_size = SUBTITLE_FONT_SIZE_DEFAULT;
    /**
     * color channel sequence: ABGR
     */
    uint32_t subtitle_font_color = SUBTITLE_FONT_COLOR_DEFAULT;
    /**
     * color channel sequence: ABGR
     */
    uint32_t subtitle_background_color = DIFF_ON_VR(SUBTITLE_FRAME_BG_COLOR_DEFAULT);

    bool show_boarder_around_subtitle = false;
    int subtitle_frame_width = SUBTITLE_FRAME_WIDTH_DEFAULT;
    int subtitle_frame_height = SUBTITLE_FRAME_HEIGHT_DEFAULT;

    int tcp_server_port = TCP_SERVER_DEFAULT_PORT;
    [[nodiscard]] std::optional<std::string> validate_tcp_server_port() const;

    DEFINE_HELPER_FUNC(subtitle_font_color);
    DEFINE_HELPER_FUNC(subtitle_font_size);
    DEFINE_HELPER_FUNC(show_boarder_around_subtitle);
    DEFINE_HELPER_FUNC(subtitle_frame_width);
    DEFINE_HELPER_FUNC(subtitle_frame_height);
    DEFINE_HELPER_FUNC(subtitle_background_color);
    DEFINE_HELPER_FUNC(tcp_server_port);

    std::string config_file_path = DIFF_ON_VR(CONFIG_FILE_PATH);
    /**
     * Load options to the 'settings' instance
     */
    static void load_from_yaml_file(const std::string& config_file_path);

    bool debug_mode = DEBUG_MODE;

private:
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;

    /**
     * A backup that saves settings that
     * the last time the user presses the "Apply" button
     */
    static Settings last_applied;

    bool operator==(const Settings& other) const; // NOLINT(*-overloaded-operator)

    void write_yaml_to(YAML::Emitter& o) const;
};

/**
 * Currently working settings,
 * other logic will retrieve option values from it,
 * dynamically changed by the user via Dashboard.
 */
extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
