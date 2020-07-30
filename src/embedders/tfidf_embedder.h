#pragma once

#include "embedder.h"
#include "token_indexer.h"

class TTfIdfEmbedder : public TEmbedder {
public:
    TTfIdfEmbedder(
        const std::string& vocabularyPath,
        tg::EEmbedderField field,
        size_t maxWords,
        const std::string& modelPath);

    explicit TTfIdfEmbedder(tg::TEmbedderConfig config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    TTokenIndexer TokenIndexer;
    std::vector<float> Idfs;
    bool UseMatrix;
    mutable torch::jit::script::Module SVDMatrix;
};
