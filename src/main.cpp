#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <exception>
#include <map>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "../thirdparty/fasttext/src/fasttext.h"
#include "../thirdparty/nlohmann_json/json.hpp"

#include "tinyxml2.h"

namespace po = boost::program_options;

struct Document {
    std::string Title;
    std::string Url;
    std::string SiteName;
    std::string Description;
    std::string FileName;
    std::string Language;
};

std::string DetectLanguage(const fasttext::FastText& model, const Document& document) {
    std::istringstream ifs(document.Title);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, 0.0);
    if (predictions.empty()) {
        return "en";
    }
    const std::string& label = predictions[0].second;
    return label.substr(label.length() - 2);
}

Document ParseFile(const char* fileName) {
    tinyxml2::XMLDocument originalDoc;
    originalDoc.LoadFile(fileName);
    const tinyxml2::XMLElement* headElement = originalDoc.FirstChildElement("html")->FirstChildElement("head");
    const tinyxml2::XMLElement* metaElement = headElement->FirstChildElement("meta");
    Document doc;
    doc.FileName = fileName;
    while (metaElement != 0) {
        const char* property = metaElement->Attribute("property");
        const char* content = metaElement->Attribute("content");
        if (content == 0 || property == 0) {
            metaElement = metaElement->NextSiblingElement("meta");
            continue;
        }
        if (std::string(property) == "og:title") {
            doc.Title = content;
        }
        if (std::string(property) == "og:url") {
            doc.Url = content;
        }
        if (std::string(property) == "og:site_name") {
            doc.SiteName = content;
        }
        if (std::string(property) == "og:description") {
            doc.Description = content;
        }
        metaElement = metaElement->NextSiblingElement("meta");
    }
    return doc;
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
        if (!vm.count("mode") || !vm.count("source_dir")) {
            std::cerr << "Not enough arguments" << std::endl;
            return -1;
        }
        std::string mode = vm["mode"].as<std::string>();
        std::cerr << "Mode: " << mode << std::endl;
        if (mode != "languages") {
            std::cerr << "Unknown or unsupported mode!" << std::endl;
            return -1;
        }

        // Read file names
        std::cerr << "Reading file names..." << std::endl;
        std::string sourceDir = vm["source_dir"].as<std::string>();
        int nDocs = vm["ndocs"].as<int>();
        boost::filesystem::path dir(sourceDir);
        boost::filesystem::recursive_directory_iterator start(dir);
        boost::filesystem::recursive_directory_iterator end;
        std::vector<std::string> fileNames;
        for (auto it = start; it != end; it++) {
            if (boost::filesystem::is_directory(it->path())) {
                continue;
            }
            std::string path = it->path().string();
            if (path.substr(path.length() - 5) == ".html") {
                fileNames.push_back(path);
            }
            if (nDocs != -1 && fileNames.size() == nDocs) {
                break;
            }
        }
        std::cerr << "Files count: " << fileNames.size() << std::endl;

        // Load models
        std::cerr << "Loading models..." << std::endl;
        std::string langDetectModelPath = vm["lang_detect_model"].as<std::string>();
        fasttext::FastText langDetectModel;
        langDetectModel.loadModel(langDetectModelPath);
        std::cerr << "FastText lang_detect model loaded" << std::endl;

        // Parse files
        std::cerr << "Pasing " << fileNames.size() << " files..." << std::endl;
        std::vector<Document> docs;
        docs.reserve(fileNames.size());
        for (const std::string& path: fileNames) {
            docs.push_back(ParseFile(path.c_str()));
        }
        std::cerr << "Parsing completed!" << std::endl;

        // Main modes
        if (mode == "languages") {
            std::map<std::string, std::vector<std::string>> langToFiles;
            for (auto& doc : docs) {
                std::string language = DetectLanguage(langDetectModel, doc);
                doc.Language = language;
                std::string fileName = doc.FileName.substr(doc.FileName.find_last_of("/") + 1);
                langToFiles[language].push_back(fileName);
                //std::cerr << fileName << " " << doc.Title << " " << language << std::endl;
            }
            nlohmann::json outputJson = nlohmann::json::array();
            for (const auto& pair : langToFiles) {
                const std::string& language = pair.first;
                const auto& files = pair.second;
                nlohmann::json object = {
                    {"language", language},
                    {"articles", files}
                };
                outputJson.push_back(object);
            }
            std::cout << outputJson.dump(4) << std::endl;
        }
        return 0;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
