#pragma once

#include <torch/script.h>

#include <string>
#include <vector>

class TTokenIndexer {
public:
    TTokenIndexer(const std::string& vocabularyPath, size_t maxWords);

    std::vector<size_t> Index(const std::string& text) const;
    torch::Tensor IndexTorch(const std::string& text) const;
    size_t Size() const { return Vocabulary.size(); }

private:
    std::unordered_map<std::string, size_t> Vocabulary;
    size_t MaxWords = 0;
};
