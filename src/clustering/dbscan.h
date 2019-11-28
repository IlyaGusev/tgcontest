#pragma once

#include "clustering.h"

#include <functional>
#include <vector>

class Dbscan : public FastTextEmbedder, public Clustering {
public:
    Dbscan(
        const std::string& modelPath,
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
