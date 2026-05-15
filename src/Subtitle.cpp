#include <imgui.h>
#include "Subtitle.h"

std::deque<std::string> Subtitle::shown{""};

void Subtitle::append(const char* text)
{
    if (const std::string s = text; s.contains("\n"))
    {
        // finish current sentence
        shown.back().append(s.substr(0, s.find_first_of('\n')));
        // start next sentence
        shown.push_back(s.substr(s.find_first_of('\n') + 1));
    }
    else
    {
        shown.back().append(s);
    }
}

void Subtitle::draw()
{
    for (const auto& s : shown)
        ImGui::TextWrapped(s.c_str());
}
