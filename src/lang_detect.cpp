#include <sstream>
#include "lang_detect.h"
#include "document.h"

std::string DetectLanguage(const fasttext::FastText& model, const Document& document) {
    std::string sample(document.Title + " " + document.Description + " " + document.Text.substr(0, 100));
    std::replace(sample.begin(), sample.end(), '\n', ' ');
    std::istringstream ifs(sample);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, 0.0);
    if (predictions.empty()) {
        return "en";
    }
    const std::string& label = predictions[0].second;
    return label.substr(label.length() - 2);
}

