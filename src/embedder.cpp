#include "embedder.h"
#include "document.h"

#include <sstream>
#include <cassert>

#include <onmt/Tokenizer.h>

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
}

size_t TFastTextEmbedder::GetEmbeddingSize() const {
    return Model.getDimension();
}

fasttext::Vector TFastTextEmbedder::GetSentenceEmbedding(const TDocument& doc) const {
    assert(doc.PreprocessedTitle && doc.PreprocessedText);
    std::istringstream ss(doc.PreprocessedTitle.get() + " " + doc.PreprocessedText.get());
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
        return avgVector;
    } else if (Mode == AM_Min) {
        return minVector;
    } else if (Mode == AM_Max) {
        return maxVector;
    }
    assert(Mode == AM_Matrix);

    auto concatTensor = torch::zeros({1, GetEmbeddingSize() * 3});
    for (size_t i = 0; i < GetEmbeddingSize(); i++) {
        concatTensor[0][i] = avgVector[i];
        concatTensor[0][GetEmbeddingSize() + i] = maxVector[i];
        concatTensor[0][2 * GetEmbeddingSize() + i] = minVector[i];
    }

    auto torchModel = torch::jit::load(TorchModelPath);
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(concatTensor);
    at::Tensor output = torchModel.forward(inputs).toTensor()[0];

    fasttext::Vector resultVector(GetEmbeddingSize());
    for (size_t i = 0; i < GetEmbeddingSize(); i++) {
        resultVector[i] = output[i].item<double>();
    }
    return resultVector;
}
