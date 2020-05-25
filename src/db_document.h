#pragma once

#include "document.pb.h"

#include <nlohmann_json/json.hpp>

#include <string>
#include <vector>
#include <unordered_map>

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

    bool Nasty = false;

public:
    static TDbDocument FromProto(const tg::TDocumentProto& proto);
    static bool FromProtoString(const std::string& value, TDbDocument* document);
    static bool ParseFromArray(const void* data, int size, TDbDocument* document);

    tg::TDocumentProto ToProto() const;
    nlohmann::json ToJson() const;
    bool ToProtoString(std::string* protoString) const;

    bool IsRussian() const { return Language == tg::LN_RU; }
    bool IsEnglish() const { return Language == tg::LN_EN; }
    bool IsNews() const { return Category != tg::NC_NOT_NEWS && Category != tg::NC_UNDEFINED; }

    bool IsStale(uint64_t timestamp) const { return timestamp > FetchTime + Ttl; }
};
