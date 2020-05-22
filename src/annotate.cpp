#include "annotate.h"
#include "detect.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

#include <boost/algorithm/string/join.hpp>

TAnnotator::TAnnotator(
    TModelStorage&& models,
    const std::set<std::string>& languages,
    const std::unordered_map<tg::ELanguage, std::string>& embeddersPathes,
    size_t minTextLength = 20,
    bool parseLinks = false,
    bool saveNotNews = false
)
    : FastTextModels{std::move(models)}
    , Languages(languages)
    , MinTextLength(minTextLength)
    , ParseLinks(parseLinks)
    , SaveNotNews(saveNotNews)
    , Tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature)
{
    for (const auto& language : Languages) {
        FastTextEmbedders[nlohmann::json(language)] = std::make_unique<TFastTextEmbedder>(
            *FastTextModels.at(language + "_vector_model"),
            TFastTextEmbedder::AM_Matrix,
            150, // TODO: BAD
            embeddersPathes.at(nlohmann::json(language))
        );
    }
}

boost::optional<TDbDocument> TAnnotator::AnnotateHtml(const std::string& path) const {
    boost::optional<TDocument> parsedDoc = parseHtml(path);
    if (!parsedDoc) {
        return boost::none;
    }
    return annotateDocument(*parsedDoc);
}

std::vector<TDbDocument> TAnnotator::AnnotateJson(const std::string& path) const {
    std::ifstream fileStream(path);
    nlohmann::json json;
    fileStream >> json;
    std::vector<TDbDocument> docs;
    for (const nlohmann::json& obj : json) {
        TDocument doc(obj);
        TDbDocument dbDoc = annotateDocument(doc);
        docs.push_back(std::move(dbDoc));
    }
    return docs;
}

TDbDocument TAnnotator::annotateDocument(const TDocument& document) const {
    TDbDocument dbDoc;
    dbDoc.Language = DetectLanguage(*FastTextModels.at("lang_detect_model"), document);
    dbDoc.Category = DetectCategory(*FastTextModels.at(nlohmann::json(dbDoc.Language) + "_cat_detect_model"), document);
    std::string cleanTitle = preprocessText(document.Title);
    std::string cleanText = preprocessText(document.Text);
    TDbDocument::TEmbedding value = FastTextEmbedders.at(dbDoc.Language)->CalcEmbedding(cleanTitle, cleanText);
    dbDoc.Embeddings.emplace(tg::EK_CLUSTERING, std::move(value));

    dbDoc.Title = document.Title;
    dbDoc.FetchTime = document.FetchTime;
    dbDoc.PubTime = document.PubTime;
    dbDoc.FileName = document.FileName;
}

std::string TAnnotator::preprocessText(const std::string& text) const {
    std::vector<std::string> tokens;
    Tokenizer.tokenize(text, tokens);
    return boost::join(tokens, " ");
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

