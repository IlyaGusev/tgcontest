#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <algorithm>

#include <boost/program_options.hpp>
#include <fasttext.h>
#include <nlohmann_json/json.hpp>

#include "clustering/slink.h"
#include "clustering/rank_docs.h"
#include "rank/rank.h"
#include "detect.h"
#include "parser.h"
#include "timer.h"
#include "util.h"

namespace po = boost::program_options;

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
            ("rating", po::value<std::string>()->default_value("ratings/rating_merged.txt"), "rating")
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
        std::unordered_map<std::string, std::unique_ptr<fasttext::FastText>>models;
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
        TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> parsingTimer;
        std::vector<std::string> languages = vm["languages"].as<std::vector<std::string>>();
        LOG_DEBUG("Parsing " << fileNames.size() << " files...");
        std::vector<TDocument> docs;
        docs.reserve(fileNames.size() / 2);
        const auto& langDetectModel = *models.at("lang_detect_model");
        for (const std::string& path: fileNames) {
            TDocument doc;
            try {
                doc = ParseFile(path.c_str());
            } catch (...) {
                LOG_DEBUG("Bad html: " << path);
                continue;
            }
            doc.Language = DetectLanguage(langDetectModel, doc);
            if (std::find(languages.begin(), languages.end(), doc.Language) == languages.end()) {
                continue;
            }
            doc.IsNews = DetectIsNews(*models.at(doc.Language + "_news_detect_model"), doc);
            doc.Category = DetectCategory(*models.at(doc.Language + "_cat_detect_model"), doc);
            if (doc.Category == "not_news") {
                doc.IsNews = false;
            }
            if (!doc.IsNews) {
                continue;
            }
            if (doc.Text.length() < 20) {
                continue;
            }
            docs.push_back(doc);
        }
        docs.shrink_to_fit();
        LOG_DEBUG("Parsing: " << docs.size() << " documents saved, " << parsingTimer.Elapsed() << " ms");

        // Output
        if (mode == "languages") {
            nlohmann::json outputJson = nlohmann::json::array();
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (const TDocument& doc : docs) {
                langToFiles[doc.Language].push_back(GetCleanFileName(doc.FileName));
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
            std::map<std::string, std::vector<std::string>> siteToTitles;
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
                nlohmann::json object = {
                    {"url", doc.Url},
                    {"site_name", doc.SiteName},
                    {"date", doc.FetchDateTime},
                    {"title", doc.Title},
                    {"description", doc.Description},
                    {"text", doc.Text},
                    {"out_links", doc.OutLinks},
                    {"language", doc.Language},
                    {"category", doc.Category},
                    {"is_news", doc.IsNews}
                };
                outputJson.push_back(object);
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
            std::map<std::string, std::vector<std::string>> catToFiles;
            for (const TDocument& doc : docs) {
                catToFiles[doc.Category].push_back(GetCleanFileName(doc.FileName));
                LOG_DEBUG(doc.Category << "\t" << doc.Title);
            }
            for (const auto& pair : catToFiles) {
                const std::string& category = pair.first;
                const std::vector<std::string>& files = pair.second;
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
        std::stable_sort(docs.begin(), docs.end(),
            [](const TDocument& d1, const TDocument& d2) { return d1.FetchTime < d2.FetchTime; }
        );

        // Clustering
        std::unique_ptr<TClustering> ruClustering;
        std::unique_ptr<TClustering> enClustering;
        const std::string clusteringType = vm["clustering_type"].as<std::string>();
        const std::string ruMatrixPath = vm["ru_sentence_embedder_matrix"].as<std::string>();
        const std::string ruBiasPath = vm["ru_sentence_embedder_bias"].as<std::string>();
        const std::string enMatrixPath = vm["en_sentence_embedder_matrix"].as<std::string>();
        const std::string enBiasPath = vm["en_sentence_embedder_bias"].as<std::string>();
        const size_t ruMaxWords = vm["ru_clustering_max_words"].as<size_t>();
        const size_t enMaxWords = vm["en_clustering_max_words"].as<size_t>();
        TFastTextEmbedder ruEmbedder(
            *models.at("ru_vector_model"),
            TFastTextEmbedder::AM_Matrix,
            ruMaxWords,
            ruMatrixPath,
            ruBiasPath
        );

        TFastTextEmbedder enEmbedder(
            *models.at("en_vector_model"),
            TFastTextEmbedder::AM_Matrix,
            enMaxWords,
            enMatrixPath,
            enBiasPath
        );
        assert(clusteringType == "slink");
        const float ruDistanceThreshold = vm["ru_clustering_distance_threshold"].as<float>();
        ruClustering = std::unique_ptr<TClustering>(
            new TSlinkClustering(ruEmbedder, ruDistanceThreshold)
        );
        const float enDistanceThreshold = vm["en_clustering_distance_threshold"].as<float>();
        enClustering = std::unique_ptr<TClustering>(
            new TSlinkClustering(enEmbedder, enDistanceThreshold)
        );

        TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> clusteringTimer;
        std::vector<TDocument> ruDocs;
        std::vector<TDocument> enDocs;
        while (!docs.empty()) {
            const TDocument& doc = docs.back();
            if (doc.Language == "en") {
                enDocs.push_back(doc);
            } else if (doc.Language == "ru") {
                ruDocs.push_back(doc);
            }
            docs.pop_back();
        }
        docs.shrink_to_fit();
        docs.clear();

        TClustering::TClusters clusters;
        {
            const TClustering::TClusters ruClusters = ruClustering->Cluster(ruDocs);
            const TClustering::TClusters enClusters = enClustering->Cluster(enDocs);
            std::copy_if(
                ruClusters.cbegin(),
                ruClusters.cend(),
                std::back_inserter(clusters),
                [](const TNewsCluster& cluster) {
                    return cluster.size() > 0;
                }
            );
            std::copy_if(
                enClusters.cbegin(),
                enClusters.cend(),
                std::back_inserter(clusters),
                [](const TNewsCluster& cluster) {
                    return cluster.size() > 0;
                }
            );
        }
        LOG_DEBUG("Clustering: " << clusteringTimer.Elapsed() << " ms (" << clusters.size() << " clusters)");

        const auto clustersSummarized = RankClustersDocs(clusters, agencyRating, ruEmbedder, enEmbedder);
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
        } else if (mode == "top") {
            nlohmann::json outputJson = nlohmann::json::array();
            double iterTimestampPercentile = vm["iter_timestamp_percentile"].as<double>();
            uint64_t iterTimestamp = GetIterTimestamp(clusters, iterTimestampPercentile);
            const auto tops = Rank(clustersSummarized, agencyRating, iterTimestamp);
            for (auto it = tops.begin(); it != tops.end(); ++it) {
                const auto categoryName = it->first;
                nlohmann::json rubricTop = {
                    {"category", categoryName},
                    {"threads", nlohmann::json::array()}
                };
                for (const auto& cluster : it->second) {
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
        }
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
