#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H

#include <string>

/**
 * Contains subtitle text content and related data
 */
class Subtitle
{
public:
    static void append(const char* text);

    static std::string getShown()
    { return shown; }

private:
    static std::string shown;
};

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
