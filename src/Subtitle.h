#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H

#include <chrono>
#include <deque>
#include <string>
#include <utility>

class Sentence
{
public:
    explicit Sentence(std::string txt) : text(std::move(txt)) {}

    uint32_t time = std::chrono::floor<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    std::string text;

    void append(const std::string& txt)
    { text.append(txt); }

    [[nodiscard]] const char* c_str() const
    { return text.c_str(); }
};

/**
 * Contains subtitle text content and related data
 */
class Subtitle
{
public:
    static void append(const char* text);
    static void draw(uint32_t font_color, uint32_t bg_color);

private:
    static std::deque<Sentence> list;
};

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
