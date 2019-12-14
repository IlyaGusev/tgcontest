#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct TDocument {
    std::string Title;
    std::string Url;
    std::string SiteName;
    std::vector<std::string> OutLinks;
    std::string Description;
    std::string FileName;
    std::string Language;
    std::string Text;
    std::string PubDateTime;
    std::string FetchDateTime;
    std::string Author;
    std::string Category;
    uint64_t PubTime = 0;
    uint64_t FetchTime = 0;
    bool IsNews;
};
