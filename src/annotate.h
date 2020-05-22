#pragma once

#include "document.h"
#include "db_document.h"
#include "embedder.h"

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <fasttext.h>

using TModelStorage = std::unordered_map<std::string, std::unique_ptr<fasttext::FastText>>;

class TAnnotator {
public:
    TAnnotator(
        TModelStorage&& models,
        const std::set<std::string>& languages,
        const std::unordered_map<tg::ELanguage, std::string>& embeddersPathes,
        size_t minTextLength /*= 20 */,
        bool parseLinks /* = false */,
        bool saveNotNews /* = false */
    );

    boost::optional<TDbDocument> AnnotateHtml(const std::string& path) const;
    std::vector<TDbDocument> AnnotateJson(const std::string& path) const;

private:
    TDbDocument annotateDocument(const TDocument& document) const;
    boost::optional<TDocument> parseHtml(const std::string& path) const;
    std::string preprocessText(const std::string& text) const;

private:
    TModelStorage FastTextModels;
    std::set<std::string> Languages;
    size_t MinTextLength = 20;
    bool ParseLinks = false;
    bool SaveNotNews = false;
    onmt::Tokenizer Tokenizer;
    std::unordered_map<tg::ELanguage, std::unique_ptr<TFastTextEmbedder>> FastTextEmbedders;
};
