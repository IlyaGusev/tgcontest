#include "embedder.h"

#include <sstream>
#include <cassert>

#include <onmt/Tokenizer.h>
#include <fasttext.h>

TFastTextEmbedder::TFastTextEmbedder(
    fasttext::FastText& model
    , TFastTextEmbedder::AggregationMode mode
    , size_t maxWords
    , const std::string& modelPath
)
    : Model(model)
    , Mode(mode)
    , MaxWords(maxWords)
    , TorchModelPath(modelPath)
{
    assert(!modelPath.empty());
    TorchModel = torch::jit::load(TorchModelPath);
}

size_t TFastTextEmbedder::GetEmbeddingSize() const {
    return Model.getDimension();
}

std::vector<float> TFastTextEmbedder::CalcEmbedding(const std::string& title, const std::string& text) const {
    std::istringstream ss(title + " " + text);
    fasttext::Vector wordVector(GetEmbeddingSize());
    fasttext::Vector avgVector(GetEmbeddingSize());
    fasttext::Vector maxVector(GetEmbeddingSize());
    fasttext::Vector minVector(GetEmbeddingSize());
    std::string word;
    size_t count = 0;
    while (ss >> word) {
        if (count > MaxWords) {
            break;
        }
        Model.getWordVector(wordVector, word);
        float norm = wordVector.norm();
        if (norm < 0.0001f) {
            continue;
        }
        wordVector.mul(1.0f / norm);

        avgVector.addVector(wordVector);
        if (count == 0) {
            maxVector = wordVector;
            minVector = wordVector;
        } else {
            for (size_t i = 0; i < GetEmbeddingSize(); i++) {
                maxVector[i] = std::max(maxVector[i], wordVector[i]);
                minVector[i] = std::min(minVector[i], wordVector[i]);
            }
        }
        count += 1;
    }
    if (count > 0) {
        avgVector.mul(1.0f / static_cast<float>(count));
    }
    if (Mode == AM_Avg) {
        return std::vector<float>(avgVector.data(), avgVector.data() + avgVector.size());
    } else if (Mode == AM_Min) {
        return std::vector<float>(minVector.data(), minVector.data() + minVector.size());
    } else if (Mode == AM_Max) {
        return std::vector<float>(maxVector.data(), maxVector.data() + maxVector.size());
    }
    assert(Mode == AM_Matrix);

    int dim = static_cast<int>(GetEmbeddingSize());
    auto tensor = torch::zeros({dim * 3}, torch::requires_grad(false));
    tensor.slice(0, 0, dim) = torch::from_blob(avgVector.data(), {dim});
    tensor.slice(0, dim, 2 * dim) = torch::from_blob(maxVector.data(), {dim});
    tensor.slice(0, 2 * dim, 3 * dim) = torch::from_blob(minVector.data(), {dim});

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(tensor.unsqueeze(0));

    at::Tensor outputTensor = TorchModel.forward(inputs).toTensor().squeeze(0).contiguous();
    float* outputTensorPtr = outputTensor.data_ptr<float>();
    std::vector<float> resultVector(GetEmbeddingSize());
    for (size_t i = 0; i < GetEmbeddingSize(); i++) {
        resultVector[i] = outputTensorPtr[i];
    }
    return resultVector;
}
