#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <imgui.h>

#define SUBTITLE_FONT_SIZE_MIN 10.0F
#define SUBTITLE_FONT_SIZE_DEFAULT 20.0F
#define SUBTITLE_FONT_SIZE_MAX 50.0F

#define SUBTITLE_FONT_COLOR_DEFAULT {1.0F, 1.0F, 1.0F, 1.0F}

#define SUBTITLE_FRAME_WIDTH_MIN 500
#define SUBTITLE_FRAME_WIDTH_DEFAULT 1280
#define SUBTITLE_FRAME_WIDTH_MAX 4000

#define SUBTITLE_FRAME_HEIGHT_MIN 200
#define SUBTITLE_FRAME_HEIGHT_DEFAULT 500
#define SUBTITLE_FRAME_HEIGHT_MAX 2000

class Settings
{
public:
    Settings();
    /**
     * Options that remain in effect after being set, only apply once per changed.
     */
    static void apply_to_subtitle();
    /**
     * Similar to apply_to_subtitle
     */
    static void apply_to_dashboard();

    static float subtitle_font_color[4];
    static ImU32 get_subtitle_font_color();
    static float subtitle_font_size;
    static bool show_boarder_around_subtitle;
    static float subtitle_frame_width;
    static float subtitle_frame_height;
    static float subtitle_window_width;
    static float subtitle_window_height;

private:
    static bool dirty;
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;
};

extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
