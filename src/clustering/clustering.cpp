#include "clustering.h"

#include <sstream>

FastTextEmbedder::FastTextEmbedder(const std::string& modelPath) {
    embedder.loadModel(modelPath);
}

size_t FastTextEmbedder::GetEmbeddingSize() const {
    return embedder.getDimension();
}

fasttext::Vector FastTextEmbedder::GetSentenceEmbedding(const Document& doc) {
    fasttext::Vector embedding(FastTextEmbedder::GetEmbeddingSize());

    std::istringstream ss(doc.Title);
    embedder.getSentenceVector(ss, embedding);

    return embedding;
}
