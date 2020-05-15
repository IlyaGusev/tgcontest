#pragma once

#include <fasttext.h>
#include <Eigen/Core>
#include <torch/script.h>

struct TDocument;

class TFastTextEmbedder {
public:
    enum AggregationMode {
        AM_Avg = 0,
        AM_Max = 1,
        AM_Min = 2,
        AM_Matrix = 3
    };

    TFastTextEmbedder(
        fasttext::FastText& model,
        AggregationMode mode = AM_Avg,
        size_t maxWords = 100,
        const std::string& modelPath = "");
    virtual ~TFastTextEmbedder() = default;

    size_t GetEmbeddingSize() const;
    fasttext::Vector GetSentenceEmbedding(const TDocument& doc) const;

private:
    fasttext::FastText& Model;
    AggregationMode Mode;
    size_t MaxWords;
    Eigen::MatrixXf Matrix;
    Eigen::VectorXf Bias;
    mutable torch::jit::script::Module TorchModel;
    std::string TorchModelPath;
};
