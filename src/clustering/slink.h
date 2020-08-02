#pragma once

#include "clustering.h"
#include "config.pb.h"

#include <Eigen/Core>

class TSlinkClustering : public TClustering {
public:
    explicit TSlinkClustering(const tg::TClusteringConfig& config);

    TClusters Cluster(
        const std::vector<TDbDocument>& docs
    ) override;

private:
    Eigen::MatrixXf CalcDistances(
        const std::vector<TDbDocument>::const_iterator begin,
        const std::vector<TDbDocument>::const_iterator end,
        const std::unordered_map<tg::EEmbeddingKey, float>& embeddingKeysWeights
    ) const;
    std::vector<size_t> ClusterBatch(
        const std::vector<TDbDocument>::const_iterator begin,
        const std::vector<TDbDocument>::const_iterator end,
        const std::unordered_map<tg::EEmbeddingKey, float>& embeddingKeysWeights
    );

private:
    tg::TClusteringConfig Config;
};
