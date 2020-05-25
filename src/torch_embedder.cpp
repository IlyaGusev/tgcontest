#include "torch_embedder.h"

TTorchEmbedder::TTorchEmbedder(
    const std::string& modelPath,
    const std::string& vocabularyPath,
    tg::EEmbedderField field,
    size_t maxWords
)
    : ModelPath(modelPath)
    , TokenIndexer(vocabularyPath, maxWords)
    , Field(field)
{
    assert(!ModelPath.empty());
    Model = torch::jit::load(ModelPath);
}

std::vector<float> TTorchEmbedder::CalcEmbedding(const std::string& title, const std::string& text) const {
    std::string input;
    if (Field == tg::EF_ALL) {
        input = title + " " + text;
    } else if (Field == tg::EF_TITLE) {
        input = title;
    } else if (Field == tg::EF_TEXT) {
        input = text;
    }
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
