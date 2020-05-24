#pragma once

#include "clustering.h"
#include "config.pb.h"

#include <Eigen/Core>

class TSlinkClustering : public TClustering {
public:
    TSlinkClustering(const tg::TClusteringConfig& config);

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
    tg::TClusteringConfig Config;
};
