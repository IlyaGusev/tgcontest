#include <string>

#include "../thirdparty/fasttext/src/fasttext.h"

struct Document;

bool DetectIsNews(const fasttext::FastText& model, const Document& document);
