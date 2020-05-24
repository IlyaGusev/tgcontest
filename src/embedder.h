#pragma once

#include "db_document.h"

#include <Eigen/Core>
#include <torch/script.h>

struct TDocument;

namespace fasttext {
    class FastText;
}

class TFastTextEmbedder {
public:
    explicit TFastTextEmbedder(
        fasttext::FastText& model,
        tg::EAggregationMode mode = tg::AM_AVG,
        tg::EEmbedderField field = tg::EF_ALL,
        size_t maxWords = 100,
        const std::string& modelPath = "",
        size_t outputDim = 50);
    virtual ~TFastTextEmbedder() = default;

    size_t GetEmbeddingSize() const;
    std::vector<float> CalcEmbedding(const std::string& title, const std::string& text) const;

private:
    fasttext::FastText& Model;
    tg::EAggregationMode Mode;
    tg::EEmbedderField Field;
    size_t MaxWords;
    Eigen::MatrixXf Matrix;
    Eigen::VectorXf Bias;
    mutable torch::jit::script::Module TorchModel;
    std::string TorchModelPath;
    size_t OutputDim;
};
