#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <torch/script.h>

#include <fstream>
#include <string>
#include <vector>

class TTokenIndexer {
public:
    TTokenIndexer(const std::string& vocabularyPath, size_t maxWords) : MaxWords(maxWords) {
        std::ifstream vocabularyFile(vocabularyPath);
        std::string word;
        size_t wordIndex = 0;
        while (std::getline(vocabularyFile, word)) {
            Vocabulary[word] = wordIndex;
            wordIndex++;
        }
    }

    torch::Tensor Index(const std::string& text) const {
        std::vector<std::string> words;
        boost::split(words, text, boost::is_any_of(" "), boost::token_compress_on);
        if (words.empty()) {
            return torch::zeros({1}, torch::requires_grad(false));
        }
        torch::Tensor inputs = torch::zeros({static_cast<long long>(words.size())}, torch::dtype(torch::kLong));
        for (size_t i = 0; i < std::min(words.size(), MaxWords); i++) {
            auto it = Vocabulary.find(words[i]);
            if (it != Vocabulary.end()) {
                inputs[i] = static_cast<int>(it->second);
            } else {
                inputs[i] = 0;
            }
        }
        return inputs;
    }

private:
    std::unordered_map<std::string, size_t> Vocabulary;
    size_t MaxWords = 0;
};
