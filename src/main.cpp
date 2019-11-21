#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include "../thirdparty/fasttext/src/fasttext.h"
#include "../thirdparty/nlohmann_json/json.hpp"

#include "parser.h"
#include "lang_detect.h"

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

int main(int argc, char** argv) {
    try {
        po::options_description desc("options");
        desc.add_options()
            ("mode", po::value<std::string>()->required(), "mode")
            ("source_dir", po::value<std::string>()->required(), "source_dir")
            ("lang_detect_model", po::value<std::string>()->default_value("models/lang_detect.ftz"), "lang_detect_model")
            ("ndocs", po::value<int>()->default_value(-1), "ndocs")
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
        if (mode != "languages" && mode != "news") {
            std::cerr << "Unknown or unsupported mode!" << std::endl;
            return -1;
        }

        // Load models
        std::cerr << "Loading models..." << std::endl;
        std::string langDetectModelPath = vm["lang_detect_model"].as<std::string>();
        fasttext::FastText langDetectModel;
        langDetectModel.loadModel(langDetectModelPath);
        std::cerr << "FastText lang_detect model loaded" << std::endl;

        // Read file names
        std::cerr << "Reading file names..." << std::endl;
        std::string sourceDir = vm["source_dir"].as<std::string>();
        int nDocs = vm["ndocs"].as<int>();
        std::vector<std::string> fileNames;
        ReadFileNames(sourceDir, fileNames, nDocs);
        std::cerr << "Files count: " << fileNames.size() << std::endl;

        // Parse files and save only russian and english texts
        std::cerr << "Pasing " << fileNames.size() << " files..." << std::endl;
        std::vector<Document> docs;
        docs.reserve(fileNames.size() / 2);
        for (const std::string& path: fileNames) {
            Document doc = ParseFile(path.c_str());
            doc.Language = DetectLanguage(langDetectModel, doc);
            if (doc.Language == "ru" || doc.Language == "en") {
                docs.push_back(doc);
            }
        }
        docs.shrink_to_fit();
        std::cerr << docs.size() << " documents saved" << std::endl;

        // Pipeline
        if (mode == "languages") {
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (const Document& doc : docs) {
                std::string fileName = doc.FileName.substr(doc.FileName.find_last_of("/") + 1);
                langToFiles[doc.Language].push_back(fileName);
            }
            nlohmann::json outputJson = nlohmann::json::array();
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
        }
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
