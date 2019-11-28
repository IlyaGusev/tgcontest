#include <iostream>

#include "hclustering.h"

HierarchicalClustering::HierarchicalClustering(float distanceThreshold)
    : DistanceThreshold(distanceThreshold)
{}

void HierarchicalClustering::Cluster(const Eigen::MatrixXf& points) {
    Eigen::MatrixXf distances(points.rows(), points.rows());
    fillDistanceMatrix(points, distances);
    size_t n = points.rows();
    Labels = std::vector<size_t>(n);
    for (size_t i = 0; i < n; i++) {
        Labels[i] = i;
    }
    size_t level = 0;
    while (level < n - 1) {
        Eigen::Index minI, minJ;
        float minDistance = distances.minCoeff(&minI, &minJ);
        if (minDistance > DistanceThreshold) {
            break;
        }
        for (size_t i = 0; i < n; i++) {
            if (Labels[i] == minJ) {
                Labels[i] = minI;
            }
        }
        for (size_t j = 0; j < distances.cols(); j++) {
            if (j == minJ) {
                continue;
            }
            distances(minI, j) = std::min(distances(minJ, j), distances(minI, j));
            distances(j, minI) = distances(minI, j);
        }
        for (size_t i = 0; i < distances.cols(); i++) {
            distances(minJ, i) = 1.0f;
            distances(i, minJ) = 1.0f;
        }
        level += 1;
    }
    for (size_t i = 0; i < n; i++) {
        std::cerr << Labels[i] << " ";
    }
    std::cerr << std::endl;
}

const std::vector<size_t>& HierarchicalClustering::GetLabels() const {
    return Labels;
}

void HierarchicalClustering::fillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) {
    distances = -((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f;
    distances += distances.Identity(distances.rows(), distances.cols());
}
