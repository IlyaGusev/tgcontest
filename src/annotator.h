#pragma once

#include "config.pb.h"
#include "db_document.h"
#include "embedders/embedder.h"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>
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
        bool saveNotNews = false,
        const std::string& mode = "top",
        boost::optional<std::vector<std::string>> languages = boost::none);

    std::vector<TDbDocument> AnnotateAll(const std::vector<std::string>& fileNames, bool fromJson) const;

    boost::optional<TDbDocument> AnnotateHtml(const std::string& path) const;
    boost::optional<TDbDocument> AnnotateHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const;

private:
    boost::optional<TDbDocument> AnnotateDocument(const TDocument& document) const;

    boost::optional<TDocument> ParseHtml(const std::string& path) const;
    boost::optional<TDocument> ParseHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const;

    std::string PreprocessText(const std::string& text) const;

    void ParseConfig(const std::string& fname);

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
