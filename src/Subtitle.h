#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H

#include <deque>
#include <string>

/**
 * Contains subtitle text content and related data
 */
class Subtitle
{
public:
    static void append(const char* text);
    static void draw(uint32_t font_color, uint32_t bg_color);

private:
    static std::deque<std::string> shown;
};

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
