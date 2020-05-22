#pragma once

#include "../cluster.h"
#include "../embedder.h"

#include <Eigen/Core>

class TClustering {
public:
    TClustering(TFastTextEmbedder& embedder) : Embedder(embedder) {}
    virtual ~TClustering() = default;

    virtual TClusters Cluster(const std::vector<TDocument>& docs) = 0;

protected:
    TFastTextEmbedder& Embedder;
};
