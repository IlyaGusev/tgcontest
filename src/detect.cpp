#include "detect.h"
#include "document.h"

#include <algorithm>
#include <sstream>

#include <fasttext.h>

const size_t FT_PREFIX_LENGTH = 9; // __label__

std::pair<std::string, double> RunFasttextClf(
    const fasttext::FastText& model,
    const std::string& originalText,
    double border,
    const std::string& defaultValue)
{
    std::string text = originalText;
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::istringstream ifs(text);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, border);
    if (predictions.empty()) {
        return std::make_pair(defaultValue, 0.0);
    }
    double probability = predictions[0].first;
    std::string label = predictions[0].second.substr(FT_PREFIX_LENGTH);
    return std::make_pair(label, probability);
}

std::string DetectLanguage(const fasttext::FastText& model, const TDocument& document) {
    std::string sample(document.Title + " " + document.Description + " " + document.Text.substr(0, 100));
    auto pair = RunFasttextClf(model, sample, 0.4, "unk");
    std::string label = pair.first;
    double probability = pair.second;
    if ((label == "ru") && probability < 0.6) {
        return "tg";
    }
    return label;
}

bool DetectIsNews(const fasttext::FastText& model, const TDocument& document) {
    std::string sample(document.Title + " " + document.Text);
    return RunFasttextClf(model, sample, 0.0, "news").first == "news";
}

ENewsCategory DetectCategory(const fasttext::FastText& model, const TDocument& document) {
    std::string sample(document.Title + " " + document.Text);
    nlohmann::json category = RunFasttextClf(model, sample, 0.0, "other").first;
    return category;
}
