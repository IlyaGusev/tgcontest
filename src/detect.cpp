#include "detect.h"
#include "document.h"

#include <algorithm>
#include <sstream>

#include <boost/optional.hpp>
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

tg::ELanguage DetectLanguage(const fasttext::FastText& model, const TDocument& document) {
    std::string sample(document.Title + " " + document.Description + " " + document.Text.substr(0, 100));
    auto pair = RunFasttextClf(model, sample, 0.4);
    if (!pair) {
        return tg::LN_UNDEFINED;
    }
    const std::string& label = pair->first;
    double probability = pair->second;
    if (label == "ru" && probability >= 0.6) {
        return tg::LN_RU;
    } else if (label == "en") {
        return tg::LN_EN;
    }
    return tg::LN_OTHER;
}

tg::ECategory DetectCategory(const fasttext::FastText& model, const std::string& title, const std::string& text) {
    std::string sample(title + " " + text);
    auto pair = RunFasttextClf(model, sample, 0.0);
    if (!pair) {
        return tg::NC_UNDEFINED;
    }
    return nlohmann::json(pair->first);
}
