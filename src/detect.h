#pragma once

#include "db_document.h"

namespace fasttext {
    class FastText;
}

struct TDocument;

tg::ELanguage DetectLanguage(const fasttext::FastText& model, const TDocument& document);
tg::ECategory DetectCategory(const fasttext::FastText& model, const std::string& title, const std::string& text);
