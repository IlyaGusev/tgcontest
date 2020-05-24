#pragma once

#include "document.pb.h"

#include <nlohmann_json/json.hpp>

#include <string>
#include <vector>
#include <unordered_map>

namespace tg {
    NLOHMANN_JSON_SERIALIZE_ENUM(tg::ELanguage, {
        {tg::LN_UNDEFINED, nullptr},
        {tg::LN_RU, "ru"},
        {tg::LN_EN, "en"},
        {tg::LN_OTHER, "??"},
    })

    NLOHMANN_JSON_SERIALIZE_ENUM(tg::ECategory, {
        {tg::NC_UNDEFINED, nullptr},
        {tg::NC_ANY, "any"},
        {tg::NC_SOCIETY, "society"},
        {tg::NC_ECONOMY, "economy"},
        {tg::NC_TECHNOLOGY, "technology"},
        {tg::NC_SPORTS, "sports"},
        {tg::NC_ENTERTAINMENT, "entertainment"},
        {tg::NC_SCIENCE, "science"},
        {tg::NC_OTHER, "other"},
        {tg::NC_NOT_NEWS, "not_news"},
    })
}

class TDbDocument {
public:
    std::string FileName;
    std::string Url;
    std::string SiteName;

    uint64_t PubTime = 0;
    uint64_t FetchTime = 0;
    uint64_t Ttl = 0;

    std::string Title;
    std::string Text;
    std::string Description;

    tg::ELanguage Language;
    tg::ECategory Category;

    using TEmbedding = std::vector<float>;
    std::unordered_map<tg::EEmbeddingKey, TEmbedding> Embeddings;

    std::vector<std::string> OutLinks;

public:
    static TDbDocument FromProto(const tg::TDocumentProto& proto);
    static bool FromProtoString(const std::string& value, TDbDocument* document);

    tg::TDocumentProto ToProto() const;
    nlohmann::json ToJson() const;
    bool ToProtoString(std::string* protoString) const;

    bool IsRussian() const { return Language == tg::LN_RU; }
    bool IsEnglish() const { return Language == tg::LN_EN; }
    bool IsNews() const { return Category != tg::NC_NOT_NEWS && Category != tg::NC_UNDEFINED; }
};
