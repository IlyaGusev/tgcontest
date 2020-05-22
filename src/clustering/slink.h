#pragma once

#include "clustering.h"

#include <Eigen/Core>

class TSlinkClustering : public TClustering {
public:
    struct TConfig {
        double SmallClusterThreshold = 0.02;
        size_t SmallClusterSize = 15;
        double MediumClusterThreshold = 0.015;
        size_t MediumClusterSize = 50;
        double LargeClusterThreshold = 0.01;
        size_t LargeClusterSize = 100;

        size_t BatchSize = 10000;
        size_t BatchIntersectionSize = 2000;

        bool UseTimestampMoving = false;
        bool BanThreadsFromSameSite = false;
    };

    TSlinkClustering(const TConfig& config);

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
    TConfig Config;
};
