#include "detect.h"
#include "document.h"

#include <sstream>

#include <fasttext.h>

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

std::string DetectCategory(const fasttext::FastText& model, const Document& document) {
    std::string sample(document.Title + " " + document.Text);
    std::replace(sample.begin(), sample.end(), '\n', ' ');
    std::istringstream ifs(sample);
    std::vector<std::pair<fasttext::real, std::string>> predictions;
    model.predictLine(ifs, predictions, 1, 0.5);
    if (predictions.empty()) {
        return "other";
    }
    std::string label = predictions[0].second.substr(9);
    return label;
}

