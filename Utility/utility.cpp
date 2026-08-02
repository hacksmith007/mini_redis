#include "utility.h"
#include <fstream>
#include <cctype>
#include <algorithm>
static std::string trim(const std::string& s)
{
    auto start = s.begin();
    while (start != s.end() && std::isspace(static_cast<unsigned char>(*start)))
        ++start;

    auto end = s.end();
    do {
        --end;
    } while (end >= start && std::isspace(static_cast<unsigned char>(*end)));

    return (start <= end) ? std::string(start, end + 1) : "";
}

std::string getAttributeValue(const std::string& filename,
                              const std::string& attribute)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return "";

    std::string line;
    while (std::getline(file, line))
    {
        line = trim(line);

        // Skip blank lines and comments
        if (line.empty() || line[0] == '#')
            continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos)
            continue;

        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));

        if (key == attribute)
            return value;
    }

    return "";
}