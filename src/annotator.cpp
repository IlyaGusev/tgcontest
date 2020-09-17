#include "annotator.h"
#include "detect.h"
#include "document.h"
#include "embedders/ft_embedder.h"
#include "embedders/tfidf_embedder.h"
#include "embedders/torch_embedder.h"
#include "nasty.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

#include <boost/algorithm/string/join.hpp>
#include <tinyxml2/tinyxml2.h>

#include <optional>

static std::unique_ptr<TEmbedder> LoadEmbedder(tg::TEmbedderConfig config) {
    if (config.type() == tg::ET_FASTTEXT) {
        return std::make_unique<TFastTextEmbedder>(config);
    } else if (config.type() == tg::ET_TORCH) {
        return std::make_unique<TTorchEmbedder>(config);
    } else if (config.type() == tg::ET_TFIDF) {
        return std::make_unique<TTfIdfEmbedder>(config);
    } else {
        ENSURE(false, "Bad embedder type");
    }
}

TAnnotator::TAnnotator(
    const std::string& configPath,
    const std::vector<std::string>& languages,
    bool saveNotNews /*= false*/,
    const std::string& mode /* = top */
)
    : Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature)
    , SaveNotNews(saveNotNews)
    , Mode(mode)
{
    ::ParseConfig(configPath, Config);
    SaveTexts = Config.save_texts() || (Mode == "json");
    SaveNotNews = Config.save_not_news() || SaveNotNews;
    ComputeNasty = Config.compute_nasty();

    LOG_DEBUG("Loading models...");

    LanguageDetector.loadModel(Config.lang_detect());
    LOG_DEBUG("FastText language detector loaded");

    for (const std::string& l : languages) {
        tg::ELanguage language = FromString<tg::ELanguage>(l);
        Languages.insert(language);
    }

    for (const auto& modelConfig : Config.category_models()) {
        const tg::ELanguage language = modelConfig.language();
        // Do not load models for languages that are not presented in CLI param
        if (Languages.find(language) == Languages.end()) {
            continue;
        }
        CategoryDetectors[language].loadModel(modelConfig.path());
        LOG_DEBUG("FastText " << ToString(language) << " category detector loaded");
    }

    for (const auto& embedderConfig : Config.embedders()) {
        tg::ELanguage language = embedderConfig.language();
        if (Languages.find(language) == Languages.end()) {
            continue;
        }
        tg::EEmbeddingKey embeddingKey = embedderConfig.embedding_key();
        Embedders[{language, embeddingKey}] = LoadEmbedder(embedderConfig);
    }
}

std::vector<TDbDocument> TAnnotator::AnnotateAll(
    const std::vector<std::string>& fileNames,
    tg::EInputFormat inputFormat) const
{
    TThreadPool threadPool;
    std::vector<TDbDocument> docs;
    std::vector<std::future<std::optional<TDbDocument>>> futures;
    if (inputFormat == tg::IF_JSON) {
        std::vector<TDocument> parsedDocs;
        for (const std::string& path: fileNames) {
            std::ifstream fileStream(path);
            nlohmann::json json;
            fileStream >> json;
            for (const nlohmann::json& obj : json) {
                parsedDocs.emplace_back(obj);
            }
        }
        parsedDocs.shrink_to_fit();
        docs.reserve(parsedDocs.size());
        futures.reserve(parsedDocs.size());
        for (const TDocument& parsedDoc: parsedDocs) {
            futures.push_back(threadPool.enqueue(&TAnnotator::AnnotateDocument, this, parsedDoc));
        }
    } else if (inputFormat == tg::IF_JSONL) {
        std::vector<TDocument> parsedDocs;
        for (const std::string& path: fileNames) {
            std::ifstream fileStream(path);
            std::string record;
            while (std::getline(fileStream, record)) {
                parsedDocs.emplace_back(nlohmann::json::parse(record));
            }
        }
        parsedDocs.shrink_to_fit();
        docs.reserve(parsedDocs.size());
        futures.reserve(parsedDocs.size());
        for (const TDocument& parsedDoc: parsedDocs) {
            futures.push_back(threadPool.enqueue(&TAnnotator::AnnotateDocument, this, parsedDoc));
        }
    } else if (inputFormat == tg::IF_HTML) {
        docs.reserve(fileNames.size());
        futures.reserve(fileNames.size());
        for (const std::string& path: fileNames) {
            using TFunc = std::optional<TDbDocument>(TAnnotator::*)(const std::string&) const;
            futures.push_back(threadPool.enqueue<TFunc>(&TAnnotator::AnnotateHtml, this, path));
        }
    } else {
        ENSURE(false, "Bad input format");
    }
    for (auto& futureDoc : futures) {
        std::optional<TDbDocument> doc = futureDoc.get();
        if (!doc) {
            continue;
        }
        if (Languages.find(doc->Language) == Languages.end()) {
            continue;
        }
        if (!doc->IsFullyIndexed()) {
            continue;
        }
        if (!doc->IsNews() && !SaveNotNews) {
            continue;
        }

        docs.push_back(std::move(doc.value()));
    }
    futures.clear();
    docs.shrink_to_fit();
    return docs;
}

std::optional<TDbDocument> TAnnotator::AnnotateHtml(const std::string& path) const {
    std::optional<TDocument> parsedDoc = ParseHtml(path);
    return parsedDoc ? AnnotateDocument(*parsedDoc) : std::nullopt;
}

std::optional<TDbDocument> TAnnotator::AnnotateHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const {
    std::optional<TDocument> parsedDoc = ParseHtml(html, fileName);
    return parsedDoc ? AnnotateDocument(*parsedDoc) : std::nullopt;
}

std::optional<TDbDocument> TAnnotator::AnnotateDocument(const TDocument& document) const {
    TDbDocument dbDoc;
    dbDoc.Language = DetectLanguage(LanguageDetector, document);
    dbDoc.Url = document.Url;
    dbDoc.Host = GetHost(dbDoc.Url);
    dbDoc.SiteName = document.SiteName;
    dbDoc.Title = document.Title;
    dbDoc.FetchTime = document.FetchTime;
    dbDoc.PubTime = document.PubTime;
    dbDoc.FileName = document.FileName;

    if (SaveTexts) {
        dbDoc.Text = document.Text;
        dbDoc.Description = document.Description;
        dbDoc.OutLinks = document.OutLinks;
    }

    if (Mode == "languages") {
        return dbDoc;
    }

    if (document.Text.length() < Config.min_text_length()) {
        return dbDoc;
    }

    std::string cleanTitle = PreprocessText(document.Title);
    std::string cleanText = PreprocessText(document.Text);

    auto detectorIt = CategoryDetectors.find(dbDoc.Language);
    if (detectorIt != CategoryDetectors.end()) {
        const auto& detector = detectorIt->second;
        dbDoc.Category = DetectCategory(detector, cleanTitle, cleanText);
    }
    for (const auto& [pair, embedder]: Embedders) {
        const auto& [language, embeddingKey] = pair;
        if (language != dbDoc.Language) {
            continue;
        }
        TDbDocument::TEmbedding value = embedder->CalcEmbedding(cleanTitle, cleanText);
        dbDoc.Embeddings.emplace(embeddingKey, std::move(value));
    }
    if (ComputeNasty) {
        dbDoc.Nasty = ComputeDocumentNasty(dbDoc);
    }

    return dbDoc;
}

std::optional<TDocument> TAnnotator::ParseHtml(const std::string& path) const {
    TDocument doc;
    try {
        doc.FromHtml(path.c_str(), Config.parse_links());
    } catch (...) {
        LOG_DEBUG("Bad html: " << path);
        return std::nullopt;
    }
    return doc;
}

std::optional<TDocument> TAnnotator::ParseHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const {
    TDocument doc;
    try {
        doc.FromHtml(html, fileName, Config.parse_links());
    } catch (...) {
        LOG_DEBUG("Bad html: " << fileName);
        return std::nullopt;
    }
    if (doc.Text.length() < Config.min_text_length()) {
        return std::nullopt;
    }
    return doc;
}

std::string TAnnotator::PreprocessText(const std::string& text) const {
    std::vector<std::string> tokens;
    Tokenizer.tokenize(text, tokens);
    return boost::join(tokens, " ");
}
