#include <sstream>
#include "lang_detect.h"
#include "document.h"

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

