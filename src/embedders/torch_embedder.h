#pragma once

#include "embedder.h"
#include "token_indexer.h"

#include <torch/script.h>

class TTorchEmbedder : public TEmbedder {
public:
    TTorchEmbedder(
        const std::string& modelPath,
        const std::string& vocabularyPath,
        tg::EEmbedderField field,
        size_t maxWords);

    explicit TTorchEmbedder(tg::TEmbedderConfig config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    mutable torch::jit::script::Module Model;
    TTokenIndexer TokenIndexer;
};
