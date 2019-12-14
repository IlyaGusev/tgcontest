#pragma once

#include "../document.h"
#include "embedder.h"

#include <fasttext.h>
#include <Eigen/Core>

typedef std::vector<std::reference_wrapper<const TDocument>> TNewsCluster;

class TClustering {
public:
    using TClusters = std::vector<TNewsCluster>;

    TClustering(TFastTextEmbedder& embedder) : Embedder(embedder) {}
    virtual ~TClustering() = default;

    virtual TClusters Cluster(const std::vector<TDocument>& docs) = 0;

protected:
    TFastTextEmbedder& Embedder;
};
