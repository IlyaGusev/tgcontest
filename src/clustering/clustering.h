#pragma once

#include "../document.h"

#include <fasttext.h>

typedef std::vector<std::reference_wrapper<const Document>> NewsCluster;

class Clustering {
public:
    using Clusters = std::vector<NewsCluster>;

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
