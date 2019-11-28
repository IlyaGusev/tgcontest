#pragma once

#include "clustering.h"

#include <Eigen/Core>

class SlinkClustering : public FastTextEmbedder, public Clustering {
public:
    SlinkClustering(
        const std::string& modelPath,
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
