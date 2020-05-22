#pragma once

#include "document.h"
#include "db_document.h"

namespace fasttext {
    class FastText;
}

tg::ELanguage DetectLanguage(const fasttext::FastText& model, const TDocument& document);
tg::ECategory DetectCategory(const fasttext::FastText& model, const TDocument& document);
