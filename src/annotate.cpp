#include "annotate.h"
#include "detect.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

void Annotate(
    const std::vector<std::string>& fileNames,
    const TModelStorage& models,
    const std::set<std::string>& languages,
    std::vector<TDocument>& docs,
    size_t minTextLength)
{
    LOG_DEBUG("Annotating " << fileNames.size() << " files...");
    TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> timer;
    docs.clear();
    docs.reserve(fileNames.size() / 2);
    TThreadPool threadPool;
    const auto& langDetectModel = *models.at("lang_detect_model");
    auto annotateDocument = [&](const std::string& path) -> boost::optional<TDocument> {
        TDocument doc;
        try {
            doc.FromHtml(path.c_str());
        } catch (...) {
            LOG_DEBUG("Bad html: " << path);
            return boost::none;
        }
        if (doc.Text.length() < minTextLength) {
            return boost::none;
        }
        doc.Language = DetectLanguage(langDetectModel, doc);
        if (!doc.Language || languages.find(doc.Language.get()) == languages.end()) {
            return boost::none;
        }
        bool isNews = DetectIsNews(*models.at(*doc.Language + "_news_detect_model"), doc);
        doc.Category = isNews ? DetectCategory(*models.at(*doc.Language + "_cat_detect_model"), doc) : NC_NOT_NEWS;
        return doc;
    };

    std::vector<std::future<boost::optional<TDocument>>> futures;
    futures.reserve(fileNames.size());
    for (const std::string& path: fileNames) {
        futures.push_back(threadPool.enqueue(annotateDocument, path));
    }

    for (auto& futureDoc : futures) {
        boost::optional<TDocument> doc = futureDoc.get();
        if (!doc
            || !doc->Language
            || doc->Category == NC_UNDEFINED
            || doc->Category == NC_NOT_NEWS)
        {
            continue;
        }
        docs.push_back(std::move(doc.get()));
    }
    docs.shrink_to_fit();
    LOG_DEBUG("Annotation: " << docs.size() << " documents saved, " << timer.Elapsed() << " ms");
}

