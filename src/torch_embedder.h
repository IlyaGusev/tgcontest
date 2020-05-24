#pragma once

#include "enum.pb.h"
#include "token_indexer.h"

#include <torch/script.h>

class TTorchEmbedder {
public:
    explicit TTorchEmbedder(
        const std::string& modelPath,
        const std::string& vocabularyPath,
        tg::EEmbedderField field = tg::EF_ALL,
        size_t maxWords = 200);

    std::vector<float> CalcEmbedding(const std::string& title, const std::string& text) const;

private:
    std::string ModelPath;
    mutable torch::jit::script::Module Model;
    TTokenIndexer TokenIndexer;

    tg::EEmbedderField Field;
};
