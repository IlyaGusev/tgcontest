#include <sstream>
#include "lang_detect.h"
#include "document.h"

std::string DetectLanguage(const fasttext::FastText& model, const Document& document) {
    std::string sample(document.Title + " " + document.Description + " " + document.Text.substr(0, 100));
    std::replace(sample.begin(), sample.end(), '\n', ' ');
    std::istringstream ifs(sample);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, 0.5);
    if (predictions.empty()) {
        return "unk";
    }
    const std::string& fullLabel = predictions[0].second;
    std::string label = fullLabel.substr(fullLabel.length() - 2);
    if ((label == "ru") && predictions[0].first < 0.7) {
        return "tg";
    }
    return label;
}

