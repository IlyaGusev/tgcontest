#pragma once

#include "config.pb.h"
#include "db_document.h"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <boost/program_options.hpp>
#include <fasttext.h>
#include <onmt/Tokenizer.h>

struct TDocument;
class TFastTextEmbedder;

using TFTModelStorage = std::unordered_map<tg::ELanguage, fasttext::FastText>;

class TAnnotator {
public:
    TAnnotator(const std::string& configPath, bool saveNotNews = false, bool forceSaveTexts = false);

    std::vector<TDbDocument> AnnotateAll(const std::vector<std::string>& fileNames, bool fromJson) const;
    boost::optional<TDbDocument> AnnotateHtml(const std::string& path) const;

private:
    boost::optional<TDbDocument> AnnotateDocument(const TDocument& document) const;
    boost::optional<TDocument> ParseHtml(const std::string& path) const;
    std::string PreprocessText(const std::string& text) const;
    void ParseConfig(const std::string& fname);

private:
    tg::TAnnotatorConfig Config;

    std::unordered_set<tg::ELanguage> Languages;
    onmt::Tokenizer Tokenizer;

    fasttext::FastText LanguageDetector;
    TFTModelStorage VectorModels;
    TFTModelStorage CategoryDetectors;
    std::unordered_map<tg::ELanguage, std::unique_ptr<TFastTextEmbedder>> Embedders;

    bool SaveNotNews = false;
    bool SaveTexts = false;
};
