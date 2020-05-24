#include "agency_rating.h"
#include "annotator.h"
#include "clusterer.h"
#include "rank.h"
#include "run_server.h"
#include "timer.h"
#include "util.h"

#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <fasttext.h>

namespace po = boost::program_options;

int main(int argc, char** argv) {
    try {
        po::options_description desc("options");
        desc.add_options()
            ("mode", po::value<std::string>()->required(), "mode")
            ("input", po::value<std::string>()->required(), "input")
            ("server_config", po::value<std::string>()->default_value("configs/server.pbtxt"), "server_config")
            ("annotator_config", po::value<std::string>()->default_value("configs/annotator.pbtxt"), "annotator_config")
            ("clusterer_config", po::value<std::string>()->default_value("configs/clusterer.pbtxt"), "clusterer_config")
            ("ndocs", po::value<int>()->default_value(-1), "ndocs")
            ("from_json", po::bool_switch()->default_value(false), "from_json")
            ("save_not_news", po::bool_switch()->default_value(false), "save_not_news")
            ("languages", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{"ru", "en"}, "ru en"), "languages")
            ("window_size", po::value<uint64_t>()->default_value(0), "window_size")
            ("print_top_debug_info", po::bool_switch()->default_value(false), "print_top_debug_info")
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
            const std::string serverConfig = vm["server_config"].as<std::string>();
            return RunServer(serverConfig);
        }

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
        const std::string annotatorConfig = vm["annotator_config"].as<std::string>();
        bool saveNotNews = vm["save_not_news"].as<bool>();
        std::vector<std::string> languages = vm["languages"].as<std::vector<std::string>>();
        TAnnotator annotator(annotatorConfig, saveNotNews, mode == "json", languages);
        TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> annotationTimer;
        std::vector<TDbDocument> docs = annotator.AnnotateAll(fileNames, fromJson);
        LOG_DEBUG("Annotation: " << annotationTimer.Elapsed() << " ms (" << docs.size() << " documents)");

        // Output
        if (mode == "languages") {
            nlohmann::json outputJson = nlohmann::json::array();
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (const TDbDocument& doc : docs) {
                langToFiles[nlohmann::json(doc.Language)].push_back(CleanFileName(doc.FileName));
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
        } else if (mode == "json") {
            nlohmann::json outputJson = nlohmann::json::array();
            for (const TDbDocument& doc : docs) {
                outputJson.push_back(doc.ToJson());
            }
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode == "news") {
            nlohmann::json articles = nlohmann::json::array();
            for (const TDbDocument& doc : docs) {
                articles.push_back(CleanFileName(doc.FileName));
            }
            nlohmann::json outputJson = nlohmann::json::object();
            outputJson["articles"] = articles;
            std::cout << outputJson.dump(4) << std::endl;
            return 0;
        } else if (mode == "categories") {
            nlohmann::json outputJson = nlohmann::json::array();

            std::vector<std::vector<std::string>> catToFiles(tg::ECategory_ARRAYSIZE);
            for (const TDbDocument& doc : docs) {
                tg::ECategory category = doc.Category;
                if (category == tg::NC_UNDEFINED || (category == tg::NC_NOT_NEWS && !saveNotNews)) {
                    continue;
                }
                catToFiles[static_cast<size_t>(category)].push_back(CleanFileName(doc.FileName));
                LOG_DEBUG(category << "\t" << doc.Title);
            }
            for (size_t i = 0; i < tg::ECategory_ARRAYSIZE; i++) {
                tg::ECategory category = static_cast<tg::ECategory>(i);
                if (category == tg::NC_UNDEFINED || category == tg::NC_ANY) {
                    continue;
                }
                if (!saveNotNews && category == tg::NC_NOT_NEWS) {
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
        const std::string clustererConfig = vm["clusterer_config"].as<std::string>();
        TClusterer clusterer(clustererConfig);
        TTimer<std::chrono::high_resolution_clock, std::chrono::milliseconds> clusteringTimer;
        TClusterIndex clusterIndex = clusterer.Cluster(docs);
        LOG_DEBUG("Clustering: " << clusteringTimer.Elapsed() << " ms")
        for (const auto& [language, langClusters]: clusterIndex.Clusters) {
            LOG_DEBUG(nlohmann::json(language) << ": " << langClusters.size() << " clusters");
        }
        if (mode == "threads") {
            nlohmann::json outputJson = nlohmann::json::array();
            for (const auto& [language, langClusters]: clusterIndex.Clusters) {
                for (const auto& cluster : langClusters) {
                    nlohmann::json files = nlohmann::json::array();
                    for (const TDbDocument& doc : cluster.GetDocuments()) {
                        files.push_back(CleanFileName(doc.FileName));
                    }
                    nlohmann::json object = {
                        {"title", cluster.GetTitle()},
                        {"articles", files}
                    };
                    outputJson.push_back(object);

                    if (cluster.GetSize() >= 2) {
                        LOG_DEBUG("\n         CLUSTER: " << cluster.GetTitle());
                        for (const TDbDocument& doc : cluster.GetDocuments()) {
                            LOG_DEBUG("  " << doc.Title << " (" << doc.Url << ")");
                        }
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
        bool printTopDebugInfo = vm["print_top_debug_info"].as<bool>();
        TClusters allClusters;
        for (const auto& language: {tg::LN_EN, tg::LN_RU}) {
            if (clusterIndex.Clusters.find(language) == clusterIndex.Clusters.end()) {
                continue;
            }
            std::copy(
                clusterIndex.Clusters.at(language).cbegin(),
                clusterIndex.Clusters.at(language).cend(),
                std::back_inserter(allClusters)
            );
        }
        const auto tops = Rank(allClusters, clusterIndex.IterTimestamp, window);
        nlohmann::json outputJson = nlohmann::json::array();
        for (auto it = tops.begin(); it != tops.end(); ++it) {
            const auto category = static_cast<tg::ECategory>(std::distance(tops.begin(), it));
            if (category == tg::NC_UNDEFINED) {
                continue;
            }
            if (!saveNotNews && category == tg::NC_NOT_NEWS) {
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
                    {"age_penalty", cluster.WeightInfo.AgePenalty},
                    {"average_us", cluster.Cluster.get().GetCountryShare().at("US")},
                    {"w_average_us", cluster.Cluster.get().GetWeightedCountryShare().at("US")}
                };
                for (const TDbDocument& doc : cluster.Cluster.get().GetDocuments()) {
                    object["articles"].push_back(CleanFileName(doc.FileName));
                }
                if (printTopDebugInfo) {
                    object["article_weights"] = nlohmann::json::array();
                    object["weight"] = cluster.WeightInfo.Weight;
                    object["importance"] = cluster.WeightInfo.Importance;
                    object["best_time"] = cluster.WeightInfo.BestTime;
                    object["age_penalty"] = cluster.WeightInfo.AgePenalty;
                    for (const auto& weight : cluster.DocWeights) {
                        object["article_weights"].push_back(weight);
                    }
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
