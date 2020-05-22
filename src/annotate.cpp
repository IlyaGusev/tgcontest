#include "annotate.h"
#include "detect.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

#include <fasttext.h>

void Annotate(
    const std::vector<std::string>& fileNames,
    const TModelStorage& models,
    const std::set<std::string>& languages,
    std::vector<TDocument>& docs,
    size_t minTextLength,
    bool parseLinks,
    bool fromJson,
    bool saveNotNews)
{
    LOG_DEBUG("Annotating " << fileNames.size() << " files...");
    TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> timer;
    docs.clear();
    docs.reserve(fileNames.size() / 2);
    TThreadPool threadPool;
    auto parseHtml = [&](const std::string& path) -> boost::optional<TDocument> {
        TDocument doc;
        try {
            doc.FromHtml(path.c_str(), parseLinks);
        } catch (...) {
            LOG_DEBUG("Bad html: " << path);
            return boost::none;
        }
        if (doc.Text.length() < minTextLength) {
            return boost::none;
        }
        return doc;
    };
    std::vector<std::future<boost::optional<TDocument>>> futures;
    if (!fromJson) {
        futures.reserve(fileNames.size());
        for (const std::string& path: fileNames) {
            futures.push_back(threadPool.enqueue(parseHtml, path));
        }
        for (auto& futureDoc : futures) {
            boost::optional<TDocument> doc = futureDoc.get();
            if (!doc) {
                continue;
            }
            docs.push_back(std::move(doc.get()));
        }
        futures.clear();
    } else {
        for (const std::string& path: fileNames) {
            std::ifstream fileStream(path);
            nlohmann::json json;
            fileStream >> json;
            for (const nlohmann::json& obj : json) {
                TDocument doc(obj);
                docs.push_back(std::move(doc));
            }
        }
    }

    const auto& langDetectModel = *models.at("lang_detect_model");
    onmt::Tokenizer tokenizer(onmt::Tokenizer::Mode::Conservative, onmt::Tokenizer::Flags::CaseFeature);
    auto annotateDocument = [&](TDocument doc) -> boost::optional<TDocument> {
        doc.Language = DetectLanguage(langDetectModel, doc);
        if (!doc.Language || languages.find(doc.Language.get()) == languages.end()) {
            return boost::none;
        }
        doc.PreprocessTextFields(tokenizer);
        doc.Category = DetectCategory(*models.at(*doc.Language + "_cat_detect_model"), doc);
        return doc;
    };
    for (const TDocument& doc: docs) {
        futures.push_back(threadPool.enqueue(annotateDocument, std::move(doc)));
    }
    docs.clear();

    for (auto& futureDoc : futures) {
        boost::optional<TDocument> doc = futureDoc.get();
        if (!doc
            || !doc->Language
            || doc->Category == NC_UNDEFINED)
        {
            continue;
        }
        if (doc->Category == NC_NOT_NEWS && !saveNotNews) {
            continue;
        }
        docs.push_back(std::move(doc.get()));
    }
    docs.shrink_to_fit();
    LOG_DEBUG("Annotation: " << docs.size() << " documents saved, " << timer.Elapsed() << " ms");
}

