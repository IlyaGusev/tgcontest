#include "tfidf_embedder.h"
#include "../util.h"

#include <sstream>
#include <cassert>

TTfIdfEmbedder::TTfIdfEmbedder(
    const std::string& vocabularyPath
    , tg::EEmbedderField field
    , size_t maxWords
    , const std::string& modelPath
)
    : TEmbedder(field)
    , TokenIndexer(vocabularyPath, maxWords)
    , Idfs(TokenIndexer.Size())
    , UseMatrix(!modelPath.empty())
{
    assert(!vocabularyPath.empty() && !modelPath.empty());
    std::ifstream vocabFile(vocabularyPath);
    std::string line;
    size_t wordIndex = 0;
    while (std::getline(vocabFile, line)) {
        std::string idfString = line.substr(line.find('\t') + 1);
        float idf = std::stof(idfString);
        Idfs[wordIndex] = idf;
        wordIndex++;
    }

    if (!modelPath.empty()) {
        SVDMatrix = torch::jit::load(modelPath);
    }

    LOG_DEBUG("TfIdf embedder " << modelPath << " loaded, " << wordIndex << " words")
}

TTfIdfEmbedder::TTfIdfEmbedder(tg::TEmbedderConfig config) : TTfIdfEmbedder(
    config.vocabulary_path(),
    config.embedder_field(),
    config.max_words() != 0 ? config.max_words() : 100,
    config.model_path()
) {}

std::vector<float> TTfIdfEmbedder::CalcEmbedding(const std::string& input) const {
    std::vector<size_t> indices = TokenIndexer.Index(input);
    std::unordered_map<size_t, size_t> wordsCounts;
    for (size_t wordIndex : indices) {
        wordsCounts[wordIndex] += 1;
    }
    std::vector<float> tfIdfVector(TokenIndexer.Size());
    for (const auto& [wordIndex, wordCount] : wordsCounts) {
        float tf = static_cast<float>(wordCount) / static_cast<float>(indices.size());
        tfIdfVector[wordIndex] = tf * Idfs[wordIndex];
    }

    if (!UseMatrix) {
        return tfIdfVector;
    }
    int dim = static_cast<int>(tfIdfVector.size());
    auto tensor = torch::zeros({dim}, torch::requires_grad(false));
    tensor.slice(0, 0, dim) = torch::from_blob(tfIdfVector.data(), {dim});

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(tensor.unsqueeze(0));

    at::Tensor outputTensor = SVDMatrix.forward(inputs).toTensor().squeeze(0).contiguous();
    float* outputTensorPtr = outputTensor.data_ptr<float>();
    size_t outputDim = outputTensor.size(0);
    std::vector<float> resultVector(outputDim);
    for (size_t i = 0; i < outputDim; i++) {
        resultVector[i] = outputTensorPtr[i];
    }
    return resultVector;
}
