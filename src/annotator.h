#pragma once

#include "config.pb.h"
#include "db_document.h"
#include "embedders/embedder.h"

#include <memory>
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <fasttext.h>
#include <onmt/Tokenizer.h>

struct TDocument;

namespace tinyxml2 {
    class XMLDocument;
}

using TFTModelStorage = std::unordered_map<tg::ELanguage, fasttext::FastText>;

class TAnnotator {
public:
    explicit TAnnotator(
        const std::string& configPath,
        const std::vector<std::string>& languages,
        bool saveNotNews = false,
        const std::string& mode = "top");

    std::vector<TDbDocument> AnnotateAll(const std::vector<std::string>& fileNames, tg::EInputFormat inputFormat) const;

    std::optional<TDbDocument> AnnotateHtml(const std::string& path) const;
    std::optional<TDbDocument> AnnotateHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const;

private:
    std::optional<TDbDocument> AnnotateDocument(const TDocument& document) const;

    std::optional<TDocument> ParseHtml(const std::string& path) const;
    std::optional<TDocument> ParseHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const;

    std::string PreprocessText(const std::string& text) const;

private:
    tg::TAnnotatorConfig Config;

    std::unordered_set<tg::ELanguage> Languages;
    onmt::Tokenizer Tokenizer;

    fasttext::FastText LanguageDetector;
    TFTModelStorage CategoryDetectors;
    std::map<std::pair<tg::ELanguage, tg::EEmbeddingKey>, std::unique_ptr<TEmbedder>> Embedders;

    bool SaveNotNews = false;
    bool SaveTexts = false;
    bool ComputeNasty = false;
    std::string Mode;
};
