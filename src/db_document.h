#pragma once

#include "document.pb.h"

#include <string>
#include <vector>
#include <unordered_map>

class TDbDocument {
public:
    std::string FileName;
    uint32_t PubTime = 0;
    uint32_t Ttl = 0;

    std::string Title;

    tg::ELanguage Language;
    tg::ECategory Category;

    using TEmbedding = std::vector<float>;
    std::unordered_map<tg::EEmbeddingKey, TEmbedding> Embeddings;

public:
    static TDbDocument FromProto(const tg::TDocumentProto& proto);
    static bool FromProtoString(const std::string& value, TDbDocument* document);

    tg::TDocumentProto ToProto() const;
    bool ToProtoString(std::string* protoString) const;
};
