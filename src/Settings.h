#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <imgui.h>

class Settings
{
public:
    Settings();
    static void apply_to_subtitle();
    static void apply_to_dashboard();

    static float subtitle_font_color[3];
    static ImVec4 get_subtitle_font_color();

private:
    static bool dirty;
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;
};

extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
