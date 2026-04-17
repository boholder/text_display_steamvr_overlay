#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <imgui.h>

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
    static ImVec4 get_subtitle_font_color();

    static float subtitle_font_size;

private:
    static bool dirty;
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;
};

extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
