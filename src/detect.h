#pragma once

#include <string>

struct Document;

namespace fasttext {
    class FastText;
}

std::string DetectLanguage(const fasttext::FastText& model, const Document& document);
bool DetectIsNews(const fasttext::FastText& model, const Document& document);
std::string DetectCategory(const fasttext::FastText& model, const Document& document);
