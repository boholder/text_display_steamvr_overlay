#include <imgui.h>
#include "Subtitle.h"

std::deque<Sentence> Subtitle::list{Sentence{""}};

static void clear_aged_sentences() {}

void Subtitle::append(const char* text)
{
    if (const std::string t = text; t.contains("\n"))
    {
        // finish current sentence
        list.back().append(t.substr(0, t.find_first_of('\n')));
        // start next sentence
        const Sentence s{t.substr(t.find_first_of('\n') + 1)};
        list.push_back(s);
    }
    else
    {
        list.back().append(t);
    }
}

void Subtitle::draw(const uint32_t font_color, const uint32_t bg_color)
{
    // visible = the alpha channel is not 00
    const bool bg_color_visible = (bg_color & 0xFF000000) != 0;
    const float frame_width = ImGui::GetContentRegionAvail().x;

    ImGui::PushStyleColor(ImGuiCol_Text, font_color);
    for (const auto& s : list)
    {
        const auto* const txt = s.c_str();

        // Manually draw background around text.
        // In this way the background will cover less space when the subtitle text is few words,
        // compared with filling the whole subtitle text child window.
        if (bg_color_visible)
        {
            const auto txt_space = ImGui::CalcTextSize(txt, nullptr, false, frame_width);
            auto upper_left = ImGui::GetCursorScreenPos();
            const auto lower_right = ImVec2(upper_left.x + txt_space.x, upper_left.y + txt_space.y);
            ImGui::GetWindowDrawList()->AddRectFilled(upper_left, lower_right, bg_color);
        }

        ImGui::TextWrapped(txt);
    }
    ImGui::PopStyleColor(); // ImGuiCol_Text
}
