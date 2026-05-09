#ifndef TEXT_DISPLAY_STEAMVR_OVERLAY_UTILS_H
#define TEXT_DISPLAY_STEAMVR_OVERLAY_UTILS_H

#include <fstream>
#include <numeric>
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

} // namespace util

#endif // TEXT_DISPLAY_STEAMVR_OVERLAY_UTILS_H
