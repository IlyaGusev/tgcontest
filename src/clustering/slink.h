#pragma once

#include "clustering.h"

#include <Eigen/Core>

class TSlinkClustering : public TClustering {
public:
    TSlinkClustering(
        float distanceThreshold,
        size_t batchSize = 10000,
        size_t batchIntersectionSize = 2000,
        bool useTimestampMoving = false
    );

    TClusters Cluster(
        const std::vector<TDbDocument>& docs,
        tg::EEmbeddingKey embeddingKey = tg::EK_CLUSTERING
    ) override;

private:
    void FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const;
    std::vector<size_t> ClusterBatch(
        const std::vector<TDbDocument>::const_iterator begin,
        const std::vector<TDbDocument>::const_iterator end,
        tg::EEmbeddingKey embeddingKey = tg::EK_CLUSTERING
    );

private:
    const float DistanceThreshold;
    const size_t BatchSize;
    const size_t BatchIntersectionSize;
    bool UseTimestampMoving;
};
