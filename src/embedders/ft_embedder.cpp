#include "ft_embedder.h"
#include "../util.h"

#include <sstream>
#include <cassert>

#include <onmt/Tokenizer.h>

TFastTextEmbedder::TFastTextEmbedder(
    const std::string& vectorModelPath
    , tg::EEmbedderField field
    , tg::EAggregationMode mode
    , size_t maxWords
    , const std::string& modelPath
)
    : TEmbedder(field)
    , Mode(mode)
    , MaxWords(maxWords)
{
    assert(!vectorModelPath.empty());
    VectorModel.loadModel(vectorModelPath);
    LOG_DEBUG("FastText " << vectorModelPath << " vector model loaded");

    if (!modelPath.empty()) {
        Model = torch::jit::load(modelPath);
        LOG_DEBUG("Torch " << modelPath << " model loaded");
    }
}

TFastTextEmbedder::TFastTextEmbedder(tg::TEmbedderConfig config) : TFastTextEmbedder(
    config.vector_model_path(),
    config.embedder_field(),
    config.aggregation_mode(),
    config.max_words() != 0 ? config.max_words() : 100,
    config.model_path()
) {}

std::vector<float> TFastTextEmbedder::CalcEmbedding(const std::string& input) const {
    std::istringstream ss(input);
    size_t vectorSize = VectorModel.getDimension();
    fasttext::Vector wordVector(vectorSize);
    fasttext::Vector avgVector(vectorSize);
    fasttext::Vector maxVector(vectorSize);
    fasttext::Vector minVector(vectorSize);
    std::string word;
    size_t count = 0;
    while (ss >> word) {
        if (count > MaxWords) {
            break;
        }
        VectorModel.getWordVector(wordVector, word);
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
            for (size_t i = 0; i < vectorSize; i++) {
                maxVector[i] = std::max(maxVector[i], wordVector[i]);
                minVector[i] = std::min(minVector[i], wordVector[i]);
            }
        }
        count += 1;
    }
    if (count > 0) {
        avgVector.mul(1.0f / static_cast<float>(count));
    }
    if (Mode == tg::AM_AVG) {
        return std::vector<float>(avgVector.data(), avgVector.data() + avgVector.size());
    } else if (Mode == tg::AM_MIN) {
        return std::vector<float>(minVector.data(), minVector.data() + minVector.size());
    } else if (Mode == tg::AM_MAX) {
        return std::vector<float>(maxVector.data(), maxVector.data() + maxVector.size());
    }
    assert(Mode == tg::AM_MATRIX);

    int dim = static_cast<int>(vectorSize);
    auto tensor = torch::zeros({dim * 3}, torch::requires_grad(false));
    tensor.slice(0, 0, dim) = torch::from_blob(avgVector.data(), {dim});
    tensor.slice(0, dim, 2 * dim) = torch::from_blob(maxVector.data(), {dim});
    tensor.slice(0, 2 * dim, 3 * dim) = torch::from_blob(minVector.data(), {dim});

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(tensor.unsqueeze(0));

    at::Tensor outputTensor = Model.forward(inputs).toTensor().squeeze(0).contiguous();
    float* outputTensorPtr = outputTensor.data_ptr<float>();
    size_t outputDim = outputTensor.size(0);
    std::vector<float> resultVector(outputDim);
    for (size_t i = 0; i < outputDim; i++) {
        resultVector[i] = outputTensorPtr[i];
    }
    return resultVector;
}
