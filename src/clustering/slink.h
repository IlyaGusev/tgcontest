#pragma once

#include "clustering.h"

#include <Eigen/Core>

class SlinkClustering : public FastTextEmbedder, public Clustering {
public:
    SlinkClustering(
        fasttext::FastText& model,
        float distanceThreshold,
        FastTextEmbedder::AggregationMode mode = AM_Avg,
        size_t maxWords = 100,
        const std::string& matrixPath = "",
        const std::string& biasPath = ""
    );

    Clusters Cluster(
        const std::vector<Document>& docs
    ) override;

private:
    void FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const;

private:
    const float DistanceThreshold;
};
