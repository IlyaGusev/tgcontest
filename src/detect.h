#include <string>

#include "../thirdparty/fasttext/src/fasttext.h"

struct Document;

std::string DetectLanguage(const fasttext::FastText& model, const Document& document);
bool DetectIsNews(const fasttext::FastText& model, const Document& document);
std::string DetectCategory(const fasttext::FastText& model, const Document& document);
