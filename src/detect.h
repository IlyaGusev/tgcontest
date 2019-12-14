#pragma once

#include <string>

struct TDocument;

namespace fasttext {
    class FastText;
}

std::string DetectLanguage(const fasttext::FastText& model, const TDocument& document);
bool DetectIsNews(const fasttext::FastText& model, const TDocument& document);
std::string DetectCategory(const fasttext::FastText& model, const TDocument& document);
