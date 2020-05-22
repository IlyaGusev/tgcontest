#pragma once

#include "clustering.h"
#include "../embedder.h"

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

    TSlinkClustering(TEmbedder& embedder, const TConfig& config);

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
    TConfig Config;
};
