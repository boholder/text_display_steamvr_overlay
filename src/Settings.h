#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

class Settings
{
public:
    Settings();
    static void apply_to_subtitle();
    static void apply_to_dashboard();

private:
    static bool dirty;
    static bool dirty_to_subtitle;
    static bool dirty_to_dashboard;
};

extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
