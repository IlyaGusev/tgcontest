#include "detect.h"
#include "document.h"

#include <algorithm>
#include <sstream>

#include <fasttext.h>

boost::optional<std::pair<std::string, double>> RunFasttextClf(
    const fasttext::FastText& model,
    const std::string& originalText,
    double border)
{
    std::string text = originalText;
    std::replace(text.begin(), text.end(), '\n', ' ');
    std::istringstream ifs(text);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, border);
    if (predictions.empty()) {
        return boost::none;
    }
    double probability = predictions[0].first;
    const size_t FT_PREFIX_LENGTH = 9; // __label__
    std::string label = predictions[0].second.substr(FT_PREFIX_LENGTH);
    return std::make_pair(label, probability);
}

boost::optional<std::string> DetectLanguage(const fasttext::FastText& model, const TDocument& document) {
    std::string sample(document.Title + " " + document.Description + " " + document.Text.substr(0, 100));
    auto pair = RunFasttextClf(model, sample, 0.4);
    if (!pair) {
        return boost::none;
    }
    const std::string& label = pair->first;
    double probability = pair->second;
    if ((label == "ru") && probability < 0.6) {
        return std::string("tg");
    }
    return label;
}

ENewsCategory DetectCategory(const fasttext::FastText& model, const TDocument& document) {
    if (!document.PreprocessedTitle || !document.PreprocessedText) {
        return NC_UNDEFINED;
    }
    std::string sample(document.PreprocessedTitle.get() + " " + document.PreprocessedText.get());
    auto pair = RunFasttextClf(model, sample, 0.0);
    if (!pair) {
        return NC_UNDEFINED;
    }
    nlohmann::json category = pair->first;
    return category;
}
