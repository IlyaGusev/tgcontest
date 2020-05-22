#pragma once

#include <Eigen/Core>
#include <torch/script.h>
#include <unordered_map>
#include <memory>

struct TDocument;

namespace fasttext {
    class FastText;
}

class TEmbedder {
public:
    virtual ~TEmbedder() = default;
    virtual size_t GetEmbeddingSize() const = 0;
    virtual std::vector<float> GetSentenceEmbedding(const TDocument& doc) const = 0;
};

class TFastTextEmbedder : public TEmbedder {
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
    std::vector<float> GetSentenceEmbedding(const TDocument& doc) const;

private:
    fasttext::FastText& Model;
    AggregationMode Mode;
    size_t MaxWords;
    Eigen::MatrixXf Matrix;
    Eigen::VectorXf Bias;
    mutable torch::jit::script::Module TorchModel;
    std::string TorchModelPath;
};

class TDummyEmbedder : public TEmbedder {
public:
    explicit TDummyEmbedder(const std::string& modelPath);

    size_t GetEmbeddingSize() const override {
        return EmbeddingSize;
    }

    std::vector<float> GetSentenceEmbedding(const TDocument& doc) const override;

private:
    std::unordered_map<std::string, std::vector<float>> UrlToEmbedding;
    std::vector<float> DefaultVector;
    size_t EmbeddingSize = 0;
};
