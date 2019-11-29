#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct Document {
    std::string Title;
    std::string Url;
    std::string SiteName;
    std::vector<std::string> OutLinks;
    std::string Description;
    std::string FileName;
    std::string Language;
    std::string Text;
    std::string DateTime;
    std::string Author;
    std::string Category;
    uint64_t Timestamp = 0.0;
    bool IsNews;
};
