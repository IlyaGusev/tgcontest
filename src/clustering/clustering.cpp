#include "clustering.h"

#include <sstream>

FastTextEmbedder::FastTextEmbedder(fasttext::FastText& model) : Model(model) {
}

size_t FastTextEmbedder::GetEmbeddingSize() const {
    return Model.getDimension();
}

fasttext::Vector FastTextEmbedder::GetSentenceEmbedding(const Document& doc) {
    fasttext::Vector embedding(FastTextEmbedder::GetEmbeddingSize());

    std::istringstream ss(doc.Title);
    Model.getSentenceVector(ss, embedding);

    return embedding;
}
