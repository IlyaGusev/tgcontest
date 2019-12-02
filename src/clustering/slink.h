#pragma once

#include "clustering.h"
#include "embedder.h"

#include <Eigen/Core>

class SlinkClustering : public Clustering {
public:
    SlinkClustering(
        FastTextEmbedder& embedder,
        float distanceThreshold
    );

    Clusters Cluster(
        const std::vector<Document>& docs
    ) override;

private:
    void FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const;

private:
    const float DistanceThreshold;
};
