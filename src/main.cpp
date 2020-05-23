#include "agency_rating.h"
#include "annotate.h"
#include "clustering/slink.h"
#include "document.h"
#include "rank.h"
#include "run_server.h"
#include "summarize.h"
#include "timer.h"
#include "util.h"

#include <boost/program_options.hpp>

namespace po = boost::program_options;

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
    std::cerr << "main" << std::endl;
    try {
        po::options_description desc("options");
        desc.add_options()
            ("mode", po::value<std::string>()->required(), "mode")
            ("input", po::value<std::string>()->required(), "input")
            ("config", po::value<std::string>()->default_value("configs/server.pbtxt"), "config")
            ("lang_detect_model", po::value<std::string>()->default_value("models/lang_detect.ftz"), "lang_detect_model")
            ("en_cat_detect_model", po::value<std::string>()->default_value("models/en_cat_v2.ftz"), "en_cat_detect_model")
            ("ru_cat_detect_model", po::value<std::string>()->default_value("models/ru_cat_v3_1.ftz"), "ru_cat_detect_model")
            ("en_vector_model", po::value<std::string>()->default_value("models/en_vectors_v2.bin"), "en_vector_model")
            ("ru_vector_model", po::value<std::string>()->default_value("models/ru_vectors_v2.bin"), "ru_vector_model")
            ("clustering_type", po::value<std::string>()->default_value("slink"), "clustering_type")
            ("en_clustering_distance_threshold", po::value<float>()->default_value(0.02f), "en_clustering_distance_threshold")
            ("en_clustering_max_words", po::value<size_t>()->default_value(250), "en_clustering_max_words")
            ("ru_clustering_distance_threshold", po::value<float>()->default_value(0.013f), "ru_clustering_distance_threshold")
            ("ru_clustering_max_words", po::value<size_t>()->default_value(150), "ru_clustering_max_words")
            ("en_sentence_embedder", po::value<std::string>()->default_value("models/en_sentence_embedder.pt"), "en_sentence_embedder")
            ("ru_sentence_embedder", po::value<std::string>()->default_value("models/ru_sentence_embedder.pt"), "ru_sentence_embedder")
            ("rating", po::value<std::string>()->default_value("models/pagerank_rating.txt"), "rating")
            ("alexa_rating", po::value<std::string>()->default_value("models/alexa_rating_2_fixed.txt"), "alexa_rating")
            ("ndocs", po::value<int>()->default_value(-1), "ndocs")
            ("min_text_length", po::value<size_t>()->default_value(20), "min_text_length")
            ("parse_links", po::bool_switch()->default_value(false), "parse_links")
            ("from_json", po::bool_switch()->default_value(false), "from_json")
            ("save_not_news", po::bool_switch()->default_value(false), "save_not_news")
            ("languages", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{"ru", "en"}, "ru en"), "languages")
            ("iter_timestamp_percentile", po::value<double>()->default_value(0.99), "iter_timestamp_percentile")
            ("window_size", po::value<uint64_t>()->default_value(0), "window_size")
            ;

        po::positional_options_description p;
        p.add("mode", 1);
        p.add("input", 1);

        po::command_line_parser parser{argc, argv};
        parser.options(desc).positional(p);
        po::parsed_options parsed_options = parser.run();
        po::variables_map vm;
        po::store(parsed_options, vm);
        po::notify(vm);

        // Args check
        if (!vm.count("mode") || !vm.count("input")) {
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
            "top",
            "server"
        };
        if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
            std::cerr << "Unknown or unsupported mode!" << std::endl;
            return -1;
        }

        if (mode == "server") {
            const std::string config = vm["config"].as<std::string>();
            return RunServer(config);
        }

        // Load models
        LOG_DEBUG("Loading models...");
        std::vector<std::string> modelsOptions = {
            "lang_detect_model",
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
        TAgencyRating agencyRating(ratingPath);
        LOG_DEBUG("Agency ratings loaded");

        // Load alexa agency ratings
        LOG_DEBUG("Loading alexa agency ratings...");
        const std::string alexaRatingPath = vm["alexa_rating"].as<std::string>();
        TAlexaAgencyRating alexaAgencyRating(alexaRatingPath);
        LOG_DEBUG("Alexa agency ratings loaded");


        // Read file names
        LOG_DEBUG("Reading file names...");
        int nDocs = vm["ndocs"].as<int>();
        bool fromJson = vm["from_json"].as<bool>();
        std::vector<std::string> fileNames;
        if (!fromJson) {
            std::string sourceDir = vm["input"].as<std::string>();
            ReadFileNames(sourceDir, fileNames, nDocs);
            LOG_DEBUG("Files count: " << fileNames.size());
        } else {
            std::string fileName = vm["input"].as<std::string>();
            fileNames.push_back(fileName);
            LOG_DEBUG("JSON file as input");
        }

        // Parse files and annotate with classifiers
        std::vector<std::string> l = vm["languages"].as<std::vector<std::string>>();
        std::set<std::string> languages(l.begin(), l.end());
        size_t minTextLength = vm["min_text_length"].as<size_t>();
        bool parseLinks = vm["parse_links"].as<bool>();
        bool saveNotNews = vm["save_not_news"].as<bool>();
        std::vector<TDocument> docs;
        Annotate(
            fileNames,
            models,
            languages,
            docs,
            /* minTextLength = */ minTextLength,
            /* parseLinks */ parseLinks,
            /* fromJson */ fromJson,
            /* saveNotNews */ saveNotNews);

        // Output
        if (mode == "languages") {
            nlohmann::json outputJson = nlohmann::json::array();
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (const TDocument& doc : docs) {
                langToFiles[doc.Language.get()].push_back(CleanFileName(doc.FileName));
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
                articles.push_back(CleanFileName(doc.FileName));
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
                if (category == NC_UNDEFINED || (category == NC_NOT_NEWS && !saveNotNews)) {
                    continue;
                }
                catToFiles[static_cast<size_t>(category)].push_back(CleanFileName(doc.FileName));
                LOG_DEBUG(category << "\t" << doc.Title);
            }
            for (size_t i = 0; i < NC_COUNT; i++) {
                ENewsCategory category = static_cast<ENewsCategory>(i);
                if (!saveNotNews && category == NC_NOT_NEWS) {
                    continue;
                }
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
            [](const TDocument& d1, const TDocument& d2) {
                if (d1.FetchTime == d2.FetchTime) {
                    if (d1.FileName.empty() && d2.FileName.empty()) {
                        return d1.Title.length() < d2.Title.length();
                    }
                    return d1.FileName < d2.FileName;
                }
                return d1.FetchTime < d2.FetchTime;
            }
        );
        const double iterTimestampPercentile = vm["iter_timestamp_percentile"].as<double>();
        uint64_t iterTimestamp = GetIterTimestamp(docs, iterTimestampPercentile);

        const std::string clusteringType = vm["clustering_type"].as<std::string>();
        assert(clusteringType == "slink");

        std::map<std::string, std::unique_ptr<TClustering>> clusterings;
        std::map<std::string, std::unique_ptr<TFastTextEmbedder>> embedders;
        for (const std::string& language : clusteringLanguages) {
            const std::string modelPath = vm[language + "_sentence_embedder"].as<std::string>();
            const size_t maxWords = vm[language + "_clustering_max_words"].as<size_t>();

            std::unique_ptr<TFastTextEmbedder> embedder(new TFastTextEmbedder(
                *models.at(language + "_vector_model"),
                TFastTextEmbedder::AM_Matrix,
                maxWords,
                modelPath
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
        TClusters clusters;
        for (const std::string& language : clusteringLanguages) {
            const TClusters langClusters = clusterings[language]->Cluster(lang2Docs[language]);
            std::copy_if(
                langClusters.cbegin(),
                langClusters.cend(),
                std::back_inserter(clusters),
                [](const TNewsCluster& cluster) {
                    return cluster.GetSize() > 0;
                }
            );
        }
        LOG_DEBUG("Clustering: " << clusteringTimer.Elapsed() << " ms (" << clusters.size() << " clusters)");

        //Summarization
        Summarize(clusters, agencyRating, embedders);
        if (mode == "threads") {
            nlohmann::json outputJson = nlohmann::json::array();
            for (const auto& cluster : clusters) {
                nlohmann::json files = nlohmann::json::array();
                for (const TDocument& doc : cluster.GetDocuments()) {
                    files.push_back(CleanFileName(doc.FileName));
                }
                nlohmann::json object = {
                    {"title", cluster.GetTitle()},
                    {"articles", files}
                };
                outputJson.push_back(object);

                if (cluster.GetSize() >= 2) {
                    LOG_DEBUG("\n         CLUSTER: " << cluster.GetTitle());
                    for (const TDocument& doc : cluster.GetDocuments()) {
                        LOG_DEBUG("  " << doc.Title << " (" << doc.Url << ")");
                    }
                }
            }
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode != "top") {
            assert(false);
        }

        // Ranking
        uint64_t window = vm["window_size"].as<uint64_t>();
        const auto tops = Rank(clusters, agencyRating, alexaAgencyRating, iterTimestamp, window);
        nlohmann::json outputJson = nlohmann::json::array();
        for (auto it = tops.begin(); it != tops.end(); ++it) {
            const auto category = static_cast<ENewsCategory>(std::distance(tops.begin(), it));
            if (!saveNotNews && category == NC_NOT_NEWS) {
                continue;
            }

            nlohmann::json rubricTop = {
                {"category", category},
                {"threads", nlohmann::json::array()}
            };
            for (const auto& cluster : *it) {
                nlohmann::json object = {
                    {"title", cluster.Title},
                    {"category", cluster.Category},
                    {"articles", nlohmann::json::array()},
                    {"article_weights", nlohmann::json::array()},
                    {"weight", cluster.WeightInfo.Weight},
                    {"importance", cluster.WeightInfo.Importance},
                    {"best_time", cluster.WeightInfo.BestTime},
                    {"age_penalty", cluster.WeightInfo.AgePenalty}
                };
                for (const TDocument& doc : cluster.Cluster.get().GetDocuments()) {
                    object["articles"].push_back(CleanFileName(doc.FileName));
                }
                for (const auto& weight : cluster.DocWeights) {
                    object["article_weights"].push_back(weight);
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
