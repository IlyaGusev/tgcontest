#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "../thirdparty/fasttext/src/fasttext.h"
#include "../thirdparty/nlohmann_json/json.hpp"

#include "detect.h"
#include "parser.h"
#include "clustering/dbscan.h"
#include "clustering/hclustering.h"

namespace po = boost::program_options;

void ReadFileNames(const std::string& directory, std::vector<std::string>& fileNames, int nDocs=-1) {
    boost::filesystem::path dirPath(directory);
    boost::filesystem::recursive_directory_iterator start(dirPath);
    boost::filesystem::recursive_directory_iterator end;
    for (auto it = start; it != end; it++) {
        if (boost::filesystem::is_directory(it->path())) {
            continue;
        }
        std::string path = it->path().string();
        if (path.substr(path.length() - 5) == ".html") {
            fileNames.push_back(path);
        }
        if (nDocs != -1 && fileNames.size() == static_cast<size_t>(nDocs)) {
            break;
        }
    }
}

std::string CleanText(std::string text) {
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::replace(text.begin(), text.end(), '\t', ' ');
    std::replace(text.begin(), text.end(), '\r', ' ');
    return text;
}

int main(int argc, char** argv) {
    try {
        po::options_description desc("options");
        desc.add_options()
            ("mode", po::value<std::string>()->required(), "mode")
            ("source_dir", po::value<std::string>()->required(), "source_dir")
            ("lang_detect_model", po::value<std::string>()->default_value("models/lang_detect.ftz"), "lang_detect_model")
            ("news_detect_model", po::value<std::string>()->default_value("models/news_detect.ftz"), "news_detect_model")
            ("cat_detect_model", po::value<std::string>()->default_value("models/cat_detect.ftz"), "cat_detect_model")
            ("vector_model", po::value<std::string>()->default_value("models/tg_lenta.bin"), "vector_model")
            ("clustering_type", po::value<std::string>()->default_value("hierarchical"), "clustering_type")
            ("clustering_distance_threshold", po::value<float>()->default_value(0.05f), "clustering_distance_threshold")
            ("clustering_eps", po::value<double>()->default_value(0.3), "clustering_eps")
            ("clustering_min_points", po::value<size_t>()->default_value(1), "clustering_min_points")
            ("ndocs", po::value<int>()->default_value(-1), "ndocs")
            ("languages", po::value<std::vector<std::string>>()->multitoken()->default_value(std::vector<std::string>{"ru", "en"}, "ru en"), "languages")
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
        std::cerr << "Mode: " << mode << std::endl;
        std::vector<std::string> modes = {
            "languages",
            "news",
            "sites",
            "json",
            "toloka",
            "categories",
            "threads"
        };
        if (std::find(modes.begin(), modes.end(), mode) == modes.end()) {
            std::cerr << "Unknown or unsupported mode!" << std::endl;
            return -1;
        }

        // Load models
        std::cerr << "Loading models..." << std::endl;
 
        const std::string langDetectModelPath = vm["lang_detect_model"].as<std::string>();
        fasttext::FastText langDetectModel;
        langDetectModel.loadModel(langDetectModelPath);
        std::cerr << "FastText lang_detect model loaded" << std::endl;

        const std::string newsDetectModelPath = vm["news_detect_model"].as<std::string>();
        fasttext::FastText newsDetectModel;
        newsDetectModel.loadModel(newsDetectModelPath);
        std::cerr << "FastText news_detect model loaded" << std::endl;

        const std::string catDetectModelPath = vm["cat_detect_model"].as<std::string>();
        fasttext::FastText catDetectModel;
        catDetectModel.loadModel(catDetectModelPath);
        std::cerr << "FastText cat_detect model loaded" << std::endl;

        // Read file names
        std::cerr << "Reading file names..." << std::endl;
        std::string sourceDir = vm["source_dir"].as<std::string>();
        int nDocs = vm["ndocs"].as<int>();
        std::vector<std::string> fileNames;
        ReadFileNames(sourceDir, fileNames, nDocs);
        std::cerr << "Files count: " << fileNames.size() << std::endl;

        // Parse files and filter by language
        std::vector<std::string> languages = vm["languages"].as<std::vector<std::string>>();
        std::cerr << "Parsing " << fileNames.size() << " files..." << std::endl;
        std::vector<Document> docs;
        docs.reserve(fileNames.size() / 2);
        for (const std::string& path: fileNames) {
            Document doc = ParseFile(path.c_str());
            doc.Language = DetectLanguage(langDetectModel, doc);
            if (std::find(languages.begin(), languages.end(), doc.Language) != languages.end()) {
                doc.IsNews = DetectIsNews(newsDetectModel, doc);
                doc.Category = DetectCategory(catDetectModel, doc);
                docs.push_back(doc);
            }
        }
        docs.shrink_to_fit();
        std::cerr << docs.size() << " documents saved" << std::endl;

        // Output
        nlohmann::json outputJson = nlohmann::json::array();
        if (mode == "languages") {
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (const Document& doc : docs) {
                std::string fileName = doc.FileName.substr(doc.FileName.find_last_of("/") + 1);
                langToFiles[doc.Language].push_back(fileName);
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
        } else if (mode == "sites") {
            std::map<std::string, std::vector<std::string>> siteToTitles;
            for (const Document& doc : docs) {
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
        } else if (mode == "json") {
            for (const Document& doc : docs) {
                nlohmann::json object = {
                    {"url", doc.Url},
                    {"site_name", doc.SiteName},
                    {"date", doc.DateTime},
                    {"title", doc.Title},
                    {"description", doc.Description},
                    {"text", doc.Text},
                    {"out_links", doc.OutLinks}
                };
                outputJson.push_back(object);
            }
            std::cout << outputJson.dump(4) << std::endl;
        } else if (mode == "toloka") {
            std::cout << "INPUT:url" << "\t" << "INPUT:title" << "\t" << "INPUT:text" << "\n";
            for (const Document& doc : docs) {
                std::cout << doc.Url << "\t" << CleanText(doc.Title) << "\t" << CleanText(doc.Text) << "\n";
            }
        } else if (mode == "news") {
            for (const Document& doc : docs) {
                if (doc.Language != "ru") {
                    continue;
                }
                if (!doc.IsNews) {
                    std::cout << doc.Title << std::endl;
                }
            }
        } else if (mode == "categories") {
            for (const Document& doc : docs) {
                std::cout << doc.Category << " " << doc.Title << std::endl;
            }
        } else if (mode == "threads") {
            const std::string vectorModelPath = vm["vector_model"].as<std::string>();

            std::unique_ptr<Clustering> clustering;

            const std::string clusteringType = vm["clustering_type"].as<std::string>();
            if (clusteringType == "hierarchical") {
                const float distanceThreshold = vm["clustering_distance_threshold"].as<float>();
                clustering = std::unique_ptr<Clustering>(new HierarchicalClustering(vectorModelPath, distanceThreshold));
            }
            else if (clusteringType == "dbscan") {
                const double eps = vm["clustering_eps"].as<double>();
                const size_t minPoints = vm["clustering_min_points"].as<size_t>();
                clustering = std::unique_ptr<Clustering>(new Dbscan(vectorModelPath, eps, minPoints));
            }

            const Clustering::Clusters clusters = clustering->Cluster(docs);

            for (const auto& cluster : clusters) {
                if (cluster.size() < 2) {
                    continue;
                }
                std::cout << "CLUSTER" << std::endl;
                for (const auto& doc : cluster) {
                    std::cout << "   " << doc.get().Title << " (" << doc.get().Url << ")" << std::endl;
                }
            }
        }
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
