#pragma once

#include "clustering.h"
#include "embedder.h"

#include <Eigen/Core>

class SlinkClustering : public Clustering {
public:
    SlinkClustering(
        FastTextEmbedder& embedder,
        float distanceThreshold,
        size_t batchSize = 10000,
        size_t batchIntersectionSize = 2000
    );

    Clusters Cluster(
        const std::vector<Document>& docs
    ) override;

private:
    void FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const;
    std::vector<size_t> ClusterBatch(
        const std::vector<Document>::const_iterator begin,
        const std::vector<Document>::const_iterator end
    );

private:
    const float DistanceThreshold;
    const size_t BatchSize;
    const size_t BatchIntersectionSize;
};
