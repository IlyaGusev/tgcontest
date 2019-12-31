#include <algorithm>
#include <cassert>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <fasttext.h>
#include <nlohmann_json/json.hpp>

#include "clustering/rank_docs.h"
#include "clustering/slink.h"
#include "detect.h"
#include "document.h"
#include "rank/rank.h"
#include "thread_pool.h"
#include "timer.h"
#include "util.h"

namespace po = boost::program_options;
using TModelStorage = std::unordered_map<std::string, std::unique_ptr<fasttext::FastText>>;

void Annotate(
    const std::vector<std::string>& fileNames,
    const TModelStorage& models,
    const std::set<std::string>& languages,
    std::vector<TDocument>& docs,
    size_t minTextLength = 20)
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

uint64_t GetIterTimestamp(const std::vector<TDocument>& documents, double percentile) {
    // In production ts.now() should be here.
    // In this case we have percentile of documents timestamps because of the small percent of wrong dates.
    if (documents.empty()) {
        return 0;
    }
    assert(std::is_sorted(documents.begin(), documents.end(), [](const TDocument& d1, const TDocument& d2) {
        return d1.FetchTime < d2.FetchTime;
    }));

    size_t index = std::floor(percentile * documents.size());
    return documents[index].FetchTime;
}

int main(int argc, char** argv) {
    try {
        po::options_description desc("options");
        desc.add_options()
            ("mode", po::value<std::string>()->required(), "mode")
            ("source_dir", po::value<std::string>()->required(), "source_dir")
            ("lang_detect_model", po::value<std::string>()->default_value("models/lang_detect.ftz"), "lang_detect_model")
            ("en_news_detect_model", po::value<std::string>()->default_value("models/en_news_detect.ftz"), "en_news_detect_model")
            ("ru_news_detect_model", po::value<std::string>()->default_value("models/ru_news_detect.ftz"), "ru_news_detect_model")
            ("en_cat_detect_model", po::value<std::string>()->default_value("models/en_cat_detect.ftz"), "en_cat_detect_model")
            ("ru_cat_detect_model", po::value<std::string>()->default_value("models/ru_cat_detect.ftz"), "ru_cat_detect_model")
            ("en_vector_model", po::value<std::string>()->default_value("models/en_tg_bbc_nc_vector_model.bin"), "en_vector_model")
            ("ru_vector_model", po::value<std::string>()->default_value("models/ru_tg_lenta_vector_model.bin"), "ru_vector_model")
            ("clustering_type", po::value<std::string>()->default_value("slink"), "clustering_type")
            ("en_clustering_distance_threshold", po::value<float>()->default_value(0.045f), "en_clustering_distance_threshold")
            ("en_clustering_max_words", po::value<size_t>()->default_value(100), "en_clustering_max_words")
            ("ru_clustering_distance_threshold", po::value<float>()->default_value(0.045f), "ru_clustering_distance_threshold")
            ("ru_clustering_max_words", po::value<size_t>()->default_value(100), "ru_clustering_max_words")
            ("en_sentence_embedder_matrix", po::value<std::string>()->default_value("models/en_sentence_embedder/matrix.txt"), "ru_sentence_embedder_matrix")
            ("en_sentence_embedder_bias", po::value<std::string>()->default_value("models/en_sentence_embedder/bias.txt"), "ru_sentence_embedder_bias")
            ("ru_sentence_embedder_matrix", po::value<std::string>()->default_value("models/ru_sentence_embedder/matrix.txt"), "ru_sentence_embedder_matrix")
            ("ru_sentence_embedder_bias", po::value<std::string>()->default_value("models/ru_sentence_embedder/bias.txt"), "ru_sentence_embedder_bias")
            ("rating", po::value<std::string>()->default_value("models/pagerank_rating.txt"), "rating")
            ("ndocs", po::value<int>()->default_value(-1), "ndocs")
            ("languages", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{"ru", "en"}, "ru en"), "languages")
            ("iter_timestamp_percentile", po::value<double>()->default_value(0.99), "iter_timestamp_percentile")
            ;

        po::positional_options_description p;
        p.add("mode", 1);
        p.add("source_dir", 1);

        po::command_line_parser parser{argc, argv};
        parser.options(desc).positional(p);
        po::parsed_options parsed_options = parser.run();
        po::variables_map vm;
        po::store(parsed_options, vm);
        po::notify(vm);

        // Args check
        if (!vm.count("mode") || !vm.count("source_dir")) {
            std::cerr << "Not enough arguments" << std::endl;
            return -1;
        }
        std::string mode = vm["mode"].as<std::string>();
        LOG_DEBUG("Mode: " << mode);
        std::vector<std::string> modes = {
            "languages",
            "news",
            "sites",
            "json",
            "categories",
            "threads",
            "top"
        };
        if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
            std::cerr << "Unknown or unsupported mode!" << std::endl;
            return -1;
        }

        // Load models
        LOG_DEBUG("Loading models...");
        std::vector<std::string> modelsOptions = {
            "lang_detect_model",
            "en_news_detect_model",
            "ru_news_detect_model",
            "en_cat_detect_model",
            "ru_cat_detect_model",
            "en_vector_model",
            "ru_vector_model"
        };
        TModelStorage models;
        for (const auto& optionName : modelsOptions) {
            const std::string modelPath = vm[optionName].as<std::string>();
            std::unique_ptr<fasttext::FastText> model(new fasttext::FastText());
            models.emplace(optionName, std::move(model));
            models.at(optionName)->loadModel(modelPath);
            LOG_DEBUG("FastText " << optionName << " loaded");
        }

        // Load agency ratings
        LOG_DEBUG("Loading agency ratings...");
        const std::string ratingPath = vm["rating"].as<std::string>();
        std::unordered_map<std::string, double> agencyRating = LoadRatings(ratingPath);
        LOG_DEBUG("Agency ratings loaded");

        // Read file names
        LOG_DEBUG("Reading file names...");
        std::string sourceDir = vm["source_dir"].as<std::string>();
        int nDocs = vm["ndocs"].as<int>();
        std::vector<std::string> fileNames;
        ReadFileNames(sourceDir, fileNames, nDocs);
        LOG_DEBUG("Files count: " << fileNames.size());

        // Parse files and annotate with classifiers
        std::vector<std::string> l = vm["languages"].as<std::vector<std::string>>();
        std::set<std::string> languages(l.begin(), l.end());
        std::vector<TDocument> docs;
        Annotate(fileNames, models, languages, docs);

        // Output
        if (mode == "languages") {
            nlohmann::json outputJson = nlohmann::json::array();
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (const TDocument& doc : docs) {
                langToFiles[doc.Language.get()].push_back(GetCleanFileName(doc.FileName));
            }
            for (const auto& pair : langToFiles) {
                const std::string& language = pair.first;
                const std::vector<std::string>& files = pair.second;
                nlohmann::json object = {
                    {"lang_code", language},
                    {"articles", files}
                };
                outputJson.push_back(object);
            }
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode == "sites") {
            nlohmann::json outputJson = nlohmann::json::array();
            std::unordered_map<std::string, std::vector<std::string>> siteToTitles;
            for (const TDocument& doc : docs) {
                siteToTitles[doc.SiteName].push_back(doc.Title);
            }
            for (const auto& pair : siteToTitles) {
                const std::string& site = pair.first;
                const std::vector<std::string>& titles = pair.second;
                nlohmann::json object = {
                    {"site", site},
                    {"titles", titles}
                };
                outputJson.push_back(object);
            }
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode == "json") {
            nlohmann::json outputJson = nlohmann::json::array();
            for (const TDocument& doc : docs) {
                outputJson.push_back(doc.ToJson());
            }
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode == "news") {
            nlohmann::json articles = nlohmann::json::array();
            for (const TDocument& doc : docs) {
                articles.push_back(GetCleanFileName(doc.FileName));
            }
            nlohmann::json outputJson = nlohmann::json::object();
            outputJson["articles"] = articles;
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode == "categories") {
            nlohmann::json outputJson = nlohmann::json::array();
            std::vector<std::vector<std::string>> catToFiles(NC_COUNT);
            for (const TDocument& doc : docs) {
                ENewsCategory category = doc.Category;
                if (category == NC_UNDEFINED || category == NC_NOT_NEWS) {
                    continue;
                }
                catToFiles[static_cast<size_t>(category)].push_back(GetCleanFileName(doc.FileName));
                LOG_DEBUG(category << "\t" << doc.Title);
            }
            for (size_t i = 0; i < NC_COUNT; i++) {
                ENewsCategory category = static_cast<ENewsCategory>(i);
                const std::vector<std::string>& files = catToFiles[i];
                nlohmann::json object = {
                    {"category", category},
                    {"articles", files}
                };
                outputJson.push_back(object);
            }
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode != "threads" && mode != "top") {
            assert(false);
        }

        // Clustering
        const std::set<std::string> clusteringLanguages = {"ru", "en"};
        for (const auto& language : languages) {
            if (clusteringLanguages.find(language) == clusteringLanguages.end()) {
                LOG_DEBUG("Language '" << language << "' is not supported for clustering!");
            }
        }
        std::stable_sort(docs.begin(), docs.end(),
            [](const TDocument& d1, const TDocument& d2) { return d1.FetchTime < d2.FetchTime; }
        );
        const double iterTimestampPercentile = vm["iter_timestamp_percentile"].as<double>();
        uint64_t iterTimestamp = GetIterTimestamp(docs, iterTimestampPercentile);

        const std::string clusteringType = vm["clustering_type"].as<std::string>();
        assert(clusteringType == "slink");

        std::map<std::string, std::unique_ptr<TClustering>> clusterings;
        std::map<std::string, std::unique_ptr<TFastTextEmbedder>> embedders;
        for (const std::string& language : clusteringLanguages) {
            const std::string matrixPath = vm[language + "_sentence_embedder_matrix"].as<std::string>();
            const std::string biasPath = vm[language + "_sentence_embedder_bias"].as<std::string>();
            const size_t maxWords = vm[language + "_clustering_max_words"].as<size_t>();

            std::unique_ptr<TFastTextEmbedder> embedder(new TFastTextEmbedder(
                *models.at(language + "_vector_model"),
                TFastTextEmbedder::AM_Matrix,
                maxWords,
                matrixPath,
                biasPath
            ));
            embedders[language] = std::move(embedder);
            const float distanceThreshold = vm[language+"_clustering_distance_threshold"].as<float>();
            std::unique_ptr<TClustering> clustering(
                new TSlinkClustering(*embedders[language], distanceThreshold)
            );
            clusterings[language] = std::move(clustering);
        }

        std::map<std::string, std::vector<TDocument>> lang2Docs;
        while (!docs.empty()) {
            const TDocument& doc = docs.back();
            assert(doc.Language);
            const std::string& language = doc.Language.get();
            if (clusteringLanguages.find(language) != clusteringLanguages.end()) {
                lang2Docs[language].push_back(doc);
            }
            docs.pop_back();
        }
        docs.shrink_to_fit();
        docs.clear();

        TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> clusteringTimer;
        TClustering::TClusters clusters;
        for (const std::string& language : clusteringLanguages) {
            const TClustering::TClusters langClusters = clusterings[language]->Cluster(lang2Docs[language]);
            std::copy_if(
                langClusters.cbegin(),
                langClusters.cend(),
                std::back_inserter(clusters),
                [](const TNewsCluster& cluster) {
                    return cluster.size() > 0;
                }
            );
        }
        LOG_DEBUG("Clustering: " << clusteringTimer.Elapsed() << " ms (" << clusters.size() << " clusters)");

        const auto clustersSummarized = RankClustersDocs(clusters, agencyRating, *embedders["ru"], *embedders["en"]);
        if (mode == "threads") {
            nlohmann::json outputJson = nlohmann::json::array();
            for (const auto& cluster : clustersSummarized) {
                nlohmann::json files = nlohmann::json::array();
                for (const auto& doc : cluster) {
                    files.push_back(GetCleanFileName(doc.get().FileName));
                }
                nlohmann::json object = {
                    {"title", cluster[0].get().Title},
                    {"articles", files}
                };
                outputJson.push_back(object);

                if (cluster.size() >= 2) {
                    LOG_DEBUG("\n         CLUSTER: " << cluster[0].get().Title);
                    for (const auto& doc : cluster) {
                        LOG_DEBUG("  " << doc.get().Title << " (" << doc.get().Url << ")");
                    }
                }
            }
            std::cout << outputJson.dump(4) << std::endl;
        } else if (mode != "top") {
            assert(false);
        }

        // Ranking
        const auto tops = Rank(clustersSummarized, agencyRating, iterTimestamp);
        nlohmann::json outputJson = nlohmann::json::array();
        for (auto it = tops.begin(); it != tops.end(); ++it) {
            const auto category = static_cast<ENewsCategory>(std::distance(tops.begin(), it));
            nlohmann::json rubricTop = {
                {"category", category},
                {"threads", nlohmann::json::array()}
            };
            for (const auto& cluster : *it) {
                nlohmann::json object = {
                    {"title", cluster.Title},
                    {"category", cluster.Category},
                    {"articles", nlohmann::json::array()}
                };
                for (const auto& doc : cluster.Cluster.get()) {
                    object["articles"].push_back(GetCleanFileName(doc.get().FileName));
                }
                rubricTop["threads"].push_back(object);
            }
            outputJson.push_back(rubricTop);
        }
        std::cout << outputJson.dump(4) << std::endl;
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
