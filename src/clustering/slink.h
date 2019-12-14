#pragma once

#include "clustering.h"
#include "embedder.h"

#include <Eigen/Core>

class TSlinkClustering : public TClustering {
public:
    TSlinkClustering(
        TFastTextEmbedder& embedder,
        float distanceThreshold,
        size_t batchSize = 10000,
        size_t batchIntersectionSize = 2000,
        bool useTimestampMoving = false
    );

    TClusters Cluster(
        const std::vector<TDocument>& docs
    ) override;

private:
    void FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const;
    std::vector<size_t> ClusterBatch(
        const std::vector<TDocument>::const_iterator begin,
        const std::vector<TDocument>::const_iterator end
    );

private:
    const float DistanceThreshold;
    const size_t BatchSize;
    const size_t BatchIntersectionSize;
    bool UseTimestampMoving;
};
