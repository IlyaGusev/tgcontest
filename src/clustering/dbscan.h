#pragma once

#include "clustering.h"
#include "embedder.h"

#include <functional>

class Dbscan : public Clustering {
public:
    Dbscan(
        FastTextEmbedder& embedder,
        double epsilon,
        size_t minPoints
    );

    Clusters Cluster(
        const std::vector<Document>& docs
    ) override;

private:
    const double Epsilon;
    const size_t MinPoints;
};
