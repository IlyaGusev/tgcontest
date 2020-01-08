#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <boost/optional.hpp>
#include <nlohmann_json/json.hpp>
#include <onmt/Tokenizer.h>

enum ENewsCategory {
    NC_NOT_NEWS = -2,
    NC_UNDEFINED = -1,
    NC_ANY = 0,
    NC_SOCIETY,
    NC_ECONOMY,
    NC_TECHNOLOGY,
    NC_SPORTS,
    NC_ENTERTAINMENT,
    NC_SCIENCE,
    NC_OTHER,

    NC_COUNT
};

NLOHMANN_JSON_SERIALIZE_ENUM(ENewsCategory, {
    {NC_UNDEFINED, nullptr},
    {NC_ANY, "any"},
    {NC_SOCIETY, "society"},
    {NC_ECONOMY, "economy"},
    {NC_TECHNOLOGY, "technology"},
    {NC_SPORTS, "sports"},
    {NC_ENTERTAINMENT, "entertainment"},
    {NC_SCIENCE, "science"},
    {NC_OTHER, "other"},
    {NC_NOT_NEWS, "not_news"},
})

struct TDocument {
public:
    // Original fields
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

    // Calculated fields
    boost::optional<std::string> PreprocessedTitle;
    boost::optional<std::string> PreprocessedText;
    boost::optional<std::string> Language;
    ENewsCategory Category = NC_UNDEFINED;

public:
    TDocument() = default;
    TDocument(const char* fileName);
    TDocument(const nlohmann::json& json);

    nlohmann::json ToJson() const;
    void FromJson(const char* fileName);
    void FromJson(const nlohmann::json& json);
    void FromHtml(
        const char* fileName,
        bool parseLinks=false,
        bool shrinkText=false,
        size_t maxWords=200
    );
    bool IsRussian() const { return Language && Language.get() == "ru"; }
    bool IsEnglish() const { return Language && Language.get() == "en"; }
    bool IsNews() const { return Category != NC_NOT_NEWS && Category != NC_UNDEFINED; }
    void PreprocessTextFields(const onmt::Tokenizer& tokenizer);
};
