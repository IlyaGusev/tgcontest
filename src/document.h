#pragma once

#include <nlohmann_json/json.hpp>

#include <string>
#include <vector>
#include <cstdint>

namespace tinyxml2 {
    class XMLDocument;
}

// Original fields only
struct TDocument {
public:
    std::string Title;
    std::string Url;
    std::string SiteName;
    std::string Description;
    std::string FileName;
    std::string Text;
    std::string Author;

    uint64_t PubTime = 0;
    uint64_t FetchTime = 0;

    std::vector<std::string> OutLinks;

public:
    TDocument() = default;
    TDocument(const char* fileName);
    TDocument(const nlohmann::json& json);
    TDocument(const tinyxml2::XMLDocument& html, const std::string& fileName);

    nlohmann::json ToJson() const;
    void FromJson(const char* fileName);
    void FromJson(const nlohmann::json& json);
    void FromHtml(
        const char* fileName,
        bool parseLinks=false,
        bool shrinkText=false,
        size_t maxWords=200
    );
    void FromHtml(
        const tinyxml2::XMLDocument& html,
        const std::string& fileName,
        bool parseLinks=false,
        bool shrinkText=false,
        size_t maxWords=200
    );
};
