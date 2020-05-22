#include "annotate.h"
#include "detect.h"
#include "document.h"
#include "embedder.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/optional.hpp>

TAnnotator::TAnnotator(
    const boost::program_options::variables_map& vm,
    bool saveNotNews = false
)
    : Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature)
    , SaveNotNews(saveNotNews)
{
    LOG_DEBUG("Loading models...");

    for (const std::string& lang : vm["languages"].as<std::vector<std::string>>()) {
        if (lang == "ru") {
            Languages.insert(tg::LN_RU);
        } else if (lang == "en") {
            Languages.insert(tg::LN_EN);
        }
    }
    SaveTexts = vm["mode"].as<std::string>() == "json";
    MinTextLength = vm["min_text_length"].as<size_t>();
    ParseLinks = vm["parse_links"].as<bool>();

    LanguageDetector.loadModel(vm.at("lang_detect_model").as<std::string>());
    LOG_DEBUG("FastText language detector loaded");

    for (tg::ELanguage language : Languages) {
        std::string langCode = nlohmann::json(language);

        CategoryDetectors[language].loadModel(vm.at(langCode + "_cat_detect_model").as<std::string>());
        LOG_DEBUG("FastText " + langCode + " category detector loaded");

        VectorModels[language].loadModel(vm.at(langCode + "_vector_model").as<std::string>());
        LOG_DEBUG("FastText " + langCode + " vector model loaded");

        Embedders[language] = std::make_unique<TFastTextEmbedder>(
            VectorModels.at(language),
            TFastTextEmbedder::AM_Matrix,
            vm.at(langCode + "_clustering_max_words").as<size_t>(),
            vm.at(langCode + "_sentence_embedder").as<std::string>()
        );
    }
}

std::vector<TDbDocument> TAnnotator::AnnotateAll(const std::vector<std::string>& fileNames, bool fromJson) const {
    TThreadPool threadPool;
    std::vector<TDbDocument> docs;
    std::vector<std::future<boost::optional<TDbDocument>>> futures;
    if (!fromJson) {
        docs.reserve(fileNames.size());
        futures.reserve(fileNames.size());
        for (const std::string& path: fileNames) {
            futures.push_back(threadPool.enqueue(&TAnnotator::AnnotateHtml, this, path));
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
            futures.push_back(threadPool.enqueue(&TAnnotator::annotateDocument, this, parsedDoc));
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
    boost::optional<TDocument> parsedDoc = parseHtml(path);
    if (!parsedDoc) {
        return boost::none;
    }
    return annotateDocument(*parsedDoc);
}

boost::optional<TDbDocument> TAnnotator::annotateDocument(const TDocument& document) const {
    TDbDocument dbDoc;
    dbDoc.Language = DetectLanguage(LanguageDetector, document);
    if (Languages.find(dbDoc.Language) == Languages.end()) {
        return boost::none;
    }
    std::string cleanTitle = preprocessText(document.Title);
    std::string cleanText = preprocessText(document.Text);
    dbDoc.Category = DetectCategory(CategoryDetectors.at(dbDoc.Language), cleanTitle, cleanText);
    if (dbDoc.Category == tg::NC_UNDEFINED) {
        return boost::none;
    }
    if (!dbDoc.IsNews() && !SaveNotNews) {
        return boost::none;
    }
    TDbDocument::TEmbedding value = Embedders.at(dbDoc.Language)->CalcEmbedding(cleanTitle, cleanText);
    dbDoc.Embeddings.emplace(tg::EK_CLUSTERING, std::move(value));

    dbDoc.Url = document.Url;
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
    return dbDoc;
}

boost::optional<TDocument> TAnnotator::parseHtml(const std::string& path) const {
    TDocument doc;
    try {
        doc.FromHtml(path.c_str(), ParseLinks);
    } catch (...) {
        LOG_DEBUG("Bad html: " << path);
        return boost::none;
    }
    if (doc.Text.length() < MinTextLength) {
        return boost::none;
    }
    return doc;
}

std::string TAnnotator::preprocessText(const std::string& text) const {
    std::vector<std::string> tokens;
    Tokenizer.tokenize(text, tokens);
    return boost::join(tokens, " ");
}
