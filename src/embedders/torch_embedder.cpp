#include "torch_embedder.h"
#include "../util.h"

TTorchEmbedder::TTorchEmbedder(
    const std::string& modelPath,
    const std::string& vocabularyPath,
    tg::EEmbedderField field,
    size_t maxWords
)
    : TEmbedder(field)
    , TokenIndexer(vocabularyPath, maxWords)
{
    ENSURE(!modelPath.empty(), "Empty model path for Torch embedder!");
    Model = torch::jit::load(modelPath);
}

TTorchEmbedder::TTorchEmbedder(tg::TEmbedderConfig config) : TTorchEmbedder(
    config.model_path(),
    config.vocabulary_path(),
    config.embedder_field(),
    config.max_words()
) {}

std::vector<float> TTorchEmbedder::CalcEmbedding(const std::string& input) const {
    auto tensor = TokenIndexer.Index(input);
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(tensor.unsqueeze(0));
    at::Tensor outputTensor = Model.forward(inputs).toTensor().squeeze(0).contiguous();
    float* outputTensorPtr = outputTensor.data_ptr<float>();
    size_t size = outputTensor.size(0);
    std::vector<float> resultVector(size);
    for (size_t i = 0; i < size; i++) {
        resultVector[i] = outputTensorPtr[i];
    }
    return resultVector;
}
