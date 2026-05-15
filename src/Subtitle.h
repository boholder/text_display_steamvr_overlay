#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H

#include "utils.h"

#include <chrono>
#include <deque>
#include <string>
#include <utility>

class Sentence
{
public:
    explicit Sentence(std::string txt) : text(std::move(txt)) {}

    explicit Sentence(const uint64_t time, std::string txt) : time(time), text(std::move(txt)) {}

    uint64_t time = util::current_time_in_milliseconds();
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
    /**
     * Remove sentences that exist longer than the setting option.
     * Being called by draw() at SUBTITLE_CLEARING_INTERVAL interval.
     */
    static void clear_aged_sentences();
};

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_SUBTITLE_H
