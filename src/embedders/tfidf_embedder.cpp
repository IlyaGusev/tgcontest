#include "tfidf_embedder.h"
#include "../util.h"

#include <sstream>
#include <cassert>

TTfIdfEmbedder::TTfIdfEmbedder(
    const std::string& modelPath
    , tg::EEmbedderField field
    , size_t maxWords
)
    : TEmbedder(field)
    , TokenIndexer(modelPath, maxWords)
    , Idfs(TokenIndexer.Size())
{
    assert(!modelPath.empty());
    std::ifstream modelFile(modelPath);
    std::string line;
    size_t wordIndex = 0;
    while (std::getline(modelFile, line)) {
        std::string idfString = line.substr(line.find('\t') + 1);
        float idf = std::stof(idfString);
        Idfs[wordIndex] = idf;
        wordIndex++;
    }
    LOG_DEBUG("TfIdf embedder " << modelPath << " loaded, " << wordIndex << " words")
}

TTfIdfEmbedder::TTfIdfEmbedder(tg::TEmbedderConfig config) : TTfIdfEmbedder(
    config.model_path(),
    config.embedder_field(),
    config.max_words() != 0 ? config.max_words() : 100
) {}

std::vector<float> TTfIdfEmbedder::CalcEmbedding(const std::string& input) const {
    std::vector<size_t> indices = TokenIndexer.Index(input);
    std::unordered_map<size_t, size_t> wordsCounts;
    for (size_t wordIndex : indices) {
        wordsCounts[wordIndex] += 1;
    }
    std::vector<float> resultVector(TokenIndexer.Size());
    for (const auto& [wordIndex, wordCount] : wordsCounts) {
        float tf = static_cast<float>(wordCount) / static_cast<float>(indices.size());
        resultVector[wordIndex] = tf * Idfs[wordIndex];
    }
    return resultVector;
}
