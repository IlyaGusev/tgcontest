#include "annotator.h"
#include "detect.h"
#include "document.h"
#include "embedder.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <tinyxml2/tinyxml2.h>


TAnnotator::TAnnotator(
    const std::string& configPath,
    bool saveNotNews /*= false*/,
    const std::string& mode /* = top */,
    boost::optional<std::vector<std::string>> languages /* = boost::none */
)
    : Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature)
    , SaveNotNews(saveNotNews)
    , Mode(mode)
{
    ParseConfig(configPath);

    LOG_DEBUG("Loading models...");

    LanguageDetector.loadModel(Config.lang_detect());
    LOG_DEBUG("FastText language detector loaded");

    for (const auto& modelConfig : Config.category_models()) {
        const tg::ELanguage language = modelConfig.language();
        CategoryDetectors[language].loadModel(modelConfig.path());
        LOG_DEBUG("FastText " << ToString(language) << " category detector loaded");
    }

    for (const auto& modelConfig : Config.models()) {
        tg::ELanguage language = modelConfig.language();
        tg::EEmbeddingKey embeddingKey = modelConfig.embedding_key();

        if (languages) {
            bool isGoodLanguage = false;
            for (const std::string& l : languages.get()) {
                if (nlohmann::json(l) == nlohmann::json(language)) {
                    isGoodLanguage = true;
                }
            }
            if (!isGoodLanguage) {
                continue;
            }
        }
        Languages.insert(language);

        VectorModels[language].loadModel(modelConfig.vector_model());
        LOG_DEBUG("FastText " << ToString(language) << " vector model loaded");

        tg::EAggregationMode am = modelConfig.aggregation_mode();
        tg::EEmbedderField field = modelConfig.embedder_field();
        if (am == tg::AM_MATRIX) {
            Embedders[{language, embeddingKey}] = std::make_unique<TFastTextEmbedder>(
                VectorModels.at(language),
                tg::AM_MATRIX,
                field,
                modelConfig.embedder().max_words(),
                modelConfig.embedder().path(),
                modelConfig.embedder().output_dim()
            );
        } else {
            Embedders[{language, embeddingKey}] = std::make_unique<TFastTextEmbedder>(
                VectorModels.at(language),
                am,
                field
            );
        }
    }
    SaveTexts = Config.save_texts() || (Mode == "json");
}

std::vector<TDbDocument> TAnnotator::AnnotateAll(const std::vector<std::string>& fileNames, bool fromJson) const {
    TThreadPool threadPool;
    std::vector<TDbDocument> docs;
    std::vector<std::future<boost::optional<TDbDocument>>> futures;
    if (!fromJson) {
        docs.reserve(fileNames.size());
        futures.reserve(fileNames.size());
        for (const std::string& path: fileNames) {
            using TFunc = boost::optional<TDbDocument>(TAnnotator::*)(const std::string&) const;
            futures.push_back(threadPool.enqueue<TFunc>(&TAnnotator::AnnotateHtml, this, path));
        }
    } else {
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
    }
    for (auto& futureDoc : futures) {
        boost::optional<TDbDocument> doc = futureDoc.get();
        if (!doc) {
            continue;
        }
        docs.push_back(std::move(doc.get()));
    }
    futures.clear();
    docs.shrink_to_fit();
    return docs;
}

boost::optional<TDbDocument> TAnnotator::AnnotateHtml(const std::string& path) const {
    boost::optional<TDocument> parsedDoc = ParseHtml(path);
    if (!parsedDoc) {
        return boost::none;
    }
    return AnnotateDocument(*parsedDoc);
}

boost::optional<TDbDocument> TAnnotator::AnnotateHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const {
    boost::optional<TDocument> parsedDoc = ParseHtml(html, fileName);
    if (!parsedDoc) {
        return boost::none;
    }
    return AnnotateDocument(*parsedDoc);
}

boost::optional<TDbDocument> TAnnotator::AnnotateDocument(const TDocument& document) const {
    TDbDocument dbDoc;
    dbDoc.Language = DetectLanguage(LanguageDetector, document);
    if (Languages.find(dbDoc.Language) == Languages.end()) {
        return boost::none;
    }
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
        return boost::none;
    }

    std::string cleanTitle = PreprocessText(document.Title);
    std::string cleanText = PreprocessText(document.Text);
    dbDoc.Category = DetectCategory(CategoryDetectors.at(dbDoc.Language), cleanTitle, cleanText);
    if (dbDoc.Category == tg::NC_UNDEFINED) {
        return boost::none;
    }
    if (!dbDoc.IsNews() && !SaveNotNews) {
        return boost::none;
    }
    for (const auto& [pair, embedder]: Embedders) {
        const auto& [language, embeddingKey] = pair;
        if (language != dbDoc.Language) {
            continue;
        }
        TDbDocument::TEmbedding value = embedder->CalcEmbedding(cleanTitle, cleanText);
        dbDoc.Embeddings.emplace(embeddingKey, std::move(value));
    }

    return dbDoc;
}

boost::optional<TDocument> TAnnotator::ParseHtml(const std::string& path) const {
    TDocument doc;
    try {
        doc.FromHtml(path.c_str(), Config.parse_links());
    } catch (...) {
        LOG_DEBUG("Bad html: " << path);
        return boost::none;
    }
    return doc;
}

boost::optional<TDocument> TAnnotator::ParseHtml(const tinyxml2::XMLDocument& html, const std::string& fileName) const {
    TDocument doc;
    try {
        doc.FromHtml(html, fileName, Config.parse_links());
    } catch (...) {
        LOG_DEBUG("Bad html: " << fileName);
        return boost::none;
    }
    if (doc.Text.length() < Config.min_text_length()) {
        return boost::none;
    }
    return doc;
}

std::string TAnnotator::PreprocessText(const std::string& text) const {
    std::vector<std::string> tokens;
    Tokenizer.tokenize(text, tokens);
    return boost::join(tokens, " ");
}

void TAnnotator::ParseConfig(const std::string& fname) {
    const int fileDesc = open(fname.c_str(), O_RDONLY);
    ENSURE(fileDesc >= 0, "Could not open config file");
    google::protobuf::io::FileInputStream fileInput(fileDesc);
    const bool success = google::protobuf::TextFormat::Parse(&fileInput, &Config);
    ENSURE(success, "Invalid prototxt file");
}
