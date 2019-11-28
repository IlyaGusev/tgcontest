#pragma once

#include "../document.h"

#include "../../thirdparty/fasttext/src/fasttext.h"

#include <vector>

class Clustering {
public:
    using Clusters = std::vector<std::vector<std::reference_wrapper<const Document>>>;

public:
    virtual Clusters Cluster(const std::vector<Document>& docs) = 0;
    virtual ~Clustering() = default;
};

class FastTextEmbedder {
public:
    FastTextEmbedder(const std::string& modelPath);
    virtual ~FastTextEmbedder() = default;

protected:
    size_t GetEmbeddingSize() const;
    fasttext::Vector GetSentenceEmbedding(const Document& str);

private:
    fasttext::FastText embedder;
};
