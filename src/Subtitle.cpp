#include <imgui.h>
#include "Subtitle.h"

std::deque<std::string> Subtitle::shown = {};

void Subtitle::append(const char* text)
{
    const std::string s = text;
    shown.push_back(s);
}

void Subtitle::draw()
{
    for (const auto& s : shown)
        ImGui::TextWrapped(s.c_str());
}
