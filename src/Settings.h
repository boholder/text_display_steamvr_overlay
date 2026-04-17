#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <imgui.h>

#define SUBTITLE_FONT_SIZE_MIN 10.0F
#define SUBTITLE_FONT_SIZE_DEFAULT 20.0F
#define SUBTITLE_FONT_SIZE_MAX 50.0F
#define SUBTITLE_FONT_COLOR_DEFAULT {1.0F, 1.0F, 1.0F, 1.0F}

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

private:
    static bool dirty;
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;
};

extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
