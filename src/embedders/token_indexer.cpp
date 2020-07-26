#include "token_indexer.h"
#include "../util.h"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>

#include <fstream>

TTokenIndexer::TTokenIndexer(const std::string& vocabularyPath, size_t maxWords) : MaxWords(maxWords) {
    std::ifstream vocabularyFile(vocabularyPath);
    std::string line;
    size_t wordIndex = 0;
    while (std::getline(vocabularyFile, line)) {
        size_t found = line.find('\t');
        const std::string word = (found != std::string::npos) ? line.substr(0, found) : line;
        ENSURE((wordIndex != 0 || word == "<unk>"), "No <unk> in the vocabulary");
        Vocabulary[word] = wordIndex;
        wordIndex++;
    }
}

std::vector<size_t> TTokenIndexer::Index(const std::string& text) const {
    std::vector<std::string> words;
    boost::split(words, text, boost::is_any_of(" "), boost::token_compress_on);
    if (words.size() > MaxWords) {
        words.resize(MaxWords);
    }
    std::vector<size_t> result(words.size());
    for (size_t i = 0; i < words.size(); i++) {
        auto it = Vocabulary.find(words[i]);
        result[i] = (it != Vocabulary.end()) ? static_cast<int>(it->second) : 0;
    }
    return result;
}

torch::Tensor TTokenIndexer::IndexTorch(const std::string& text) const {
    std::vector<size_t> indices = Index(text);
    torch::Tensor inputs = torch::zeros({static_cast<long long>(indices.size())}, torch::dtype(torch::kLong));
    for (size_t i = 0; i < indices.size(); i++) {
        inputs[i] = static_cast<int>(indices[i]);
    }
    return inputs;
}

