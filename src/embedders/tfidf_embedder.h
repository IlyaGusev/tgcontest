#pragma once

#include "embedder.h"
#include "token_indexer.h"

class TTfIdfEmbedder : public TEmbedder {
public:
    TTfIdfEmbedder(
        const std::string& modelPath,
        tg::EEmbedderField field,
        size_t maxWords);

    explicit TTfIdfEmbedder(tg::TEmbedderConfig config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    TTokenIndexer TokenIndexer;
    std::vector<float> Idfs;
};
