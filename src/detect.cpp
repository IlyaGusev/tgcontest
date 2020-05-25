#include "detect.h"
#include "document.h"
#include "util.h"

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

bool TooManyUnknownSymbols(const TDocument& doc) {
    size_t realSize = 0;
    size_t badSymb = 0;
    size_t i = 0;

    while(i < doc.Title.size()) {
        unsigned char sym = (unsigned char) doc.Title[i];
        if (sym <= 127) {
            ++realSize;
            ++i;
        } else if (sym >= 240) { // 4 bytes utf
            ++realSize;
            ++badSymb;
            i += 4;
        } else if (sym >= 220) { // 3 bytes utf
            ++realSize;
            ++badSymb;
            i += 3;
        } else { // 2 bytes utf. may be ru
            i += 1;
            if (i >= doc.Title.size()) break;
            unsigned char sym2 = (unsigned char) doc.Title[i];

            if (((sym == 208) && (sym2 >= 144)) || ((sym == 209) && (sym2 <= 143))) {
                ++realSize;
                i += 1;
            } else {
                ++realSize;
                ++badSymb;
            }
        }
    }

    if (badSymb * 2 > realSize) {
        return true;
    }
    return false;
}

tg::ELanguage DetectLanguage(const fasttext::FastText& model, const TDocument& document) {
    std::string sample(document.Title + " " + document.Description + " " + document.Text.substr(0, 100));
    auto pair = RunFasttextClf(model, sample, 0.4);
    if (!pair) {
        return tg::LN_UNDEFINED;
    }
    const std::string& label = pair->first;
    double probability = pair->second;
    
    if (TooManyUnknownSymbols(document)) {
        return tg::LN_OTHER;
    }

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
    return FromString<tg::ECategory>(pair->first);
}
