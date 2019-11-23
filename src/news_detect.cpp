#include <sstream>
#include "news_detect.h"
#include "document.h"

bool DetectIsNews(const fasttext::FastText& model, const Document& document) {
    std::string sample(document.Title);
    std::replace(sample.begin(), sample.end(), '\n', ' ');
    std::istringstream ifs(sample);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, 0.5);
    if (predictions.empty()) {
        return true;
    }
    const std::string& fullLabel = predictions[0].second;
    std::string label = fullLabel.substr(fullLabel.length() - 1);
    return label == "1";
}

