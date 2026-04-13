#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H

#include <string>

class Settings
{
public:
    Settings()
    { t = "Hello"; }

    std::string t;
};

extern Settings settings;

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SETTINGS_H
