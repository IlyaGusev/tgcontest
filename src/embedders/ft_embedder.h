#pragma once

#include "embedder.h"

#include <Eigen/Core>
#include <fasttext.h>
#include <torch/script.h>

struct TDocument;

namespace fasttext {
    class FastText;
}

class TFastTextEmbedder : public TEmbedder {
public:
    TFastTextEmbedder(
        const std::string& vectorModelPath,
        tg::EEmbedderField field,
        tg::EAggregationMode mode,
        size_t maxWords,
        const std::string& modelPath);

    explicit TFastTextEmbedder(tg::TEmbedderConfig config);

    std::vector<float> CalcEmbedding(const std::string& input) const override;

private:
    tg::EAggregationMode Mode;
    fasttext::FastText VectorModel;
    size_t MaxWords;
    mutable torch::jit::script::Module Model;
};
