#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_UTILS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_UTILS_H

#include <fstream>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace util
{

static void save_to_file(const std::string& filename, const std::string& content)
{
    if (std::ofstream outFile(filename); outFile.is_open())
    {
        outFile << content;
        outFile.flush();
        outFile.close();
    }
    else
    {
        SPDLOG_ERROR("Unable to open file [{}]", filename);
    }
}

static std::string diff_lines(const std::string& a, const std::string& b)
{
    auto split_lines = [](const std::string& s) -> std::vector<std::string>
    {
        std::vector<std::string> lines;
        std::istringstream iss(s);
        std::string line;
        while (std::getline(iss, line))
        {
            lines.push_back(line);
        }
        return lines;
    };

    const auto a_lines = split_lines(a);
    const auto b_lines = split_lines(b);
    std::vector<std::string> diff;

    const size_t max = std::max(a_lines.size(), b_lines.size());
    for (size_t i = 0; i < max; i++)
    {
        std::string al = i < a_lines.size() ? a_lines[i] : "";
        std::string bl = i < b_lines.size() ? b_lines[i] : "";
        if (al != bl)
        {
            if (!al.empty())
                diff.push_back("- " + al);
            if (!bl.empty())
                diff.push_back("+ " + bl);
        }
    }

    std::stringstream r;
    std::ranges::copy(diff, std::ostream_iterator<std::string>(r, "\n"));
    return r.str();
}

static void color_u32_to_f4(const uint32_t color, float (&out)[4])
{
    // ColorConvertU32ToFloat4 returns ABGR
    // ref: https://github.com/ocornut/imgui/issues/761
    const ImVec4 v = ImGui::ColorConvertU32ToFloat4(color);
    out[0] = v.x;
    out[1] = v.y;
    out[2] = v.z;
    out[3] = v.w;
};

static uint32_t color_f4_to_u32(const float (&color)[4])
{ return ImGui::ColorConvertFloat4ToU32({color[0], color[1], color[2], color[3]}); };

static uint32_t revert_color_channel_order(const uint32_t color)
{
    const ImVec4 v = ImGui::ColorConvertU32ToFloat4(color);
    // revert the order
    return ImGui::ColorConvertFloat4ToU32({v.w, v.z, v.y, v.x});
}

} // namespace util

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_UTILS_H
