#pragma once

#include "../document.h"
#include "embedder.h"

#include <fasttext.h>
#include <Eigen/Core>

typedef std::vector<std::reference_wrapper<const Document>> NewsCluster;

class Clustering {
public:
    using Clusters = std::vector<NewsCluster>;

    Clustering(FastTextEmbedder& embedder) : Embedder(embedder) {}
    virtual ~Clustering() = default;

    virtual Clusters Cluster(const std::vector<Document>& docs) = 0;

protected:
    FastTextEmbedder& Embedder;
};
