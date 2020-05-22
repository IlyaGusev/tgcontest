#pragma once

#include "../cluster.h"
#include "../embedder.h"

#include <Eigen/Core>

class TClustering {
public:
    TClustering(TEmbedder& embedder) : Embedder(embedder) {}
    virtual ~TClustering() = default;

    virtual TClusters Cluster(const std::vector<TDocument>& docs) = 0;

protected:
    TEmbedder& Embedder;
};
