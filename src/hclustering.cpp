#include <iostream>
#include <cassert>

#include "hclustering.h"

HierarchicalClustering::HierarchicalClustering(float distanceThreshold)
    : DistanceThreshold(distanceThreshold)
{}

// SLINK: https://sites.cs.ucsb.edu/~veronika/MAE/summary_SLINK_Sibson72.pdf
void HierarchicalClustering::Cluster(const Eigen::MatrixXf& points) {
    Eigen::MatrixXf distances(points.rows(), points.rows());
    fillDistanceMatrix(points, distances);

    // Prepare 3 arrays
    const float INF_DISTANCE = 1.0f;
    size_t n = points.rows();
    Labels = std::vector<size_t>(n);
    for (size_t i = 0; i < n; i++) {
        Labels[i] = i;
    }
    std::vector<size_t> nn(n);
    std::vector<float> nnDistances(n);
    for (size_t i = 0; i < n; i++) {
        Eigen::Index minJ;
        nnDistances[i] = distances.row(i).minCoeff(&minJ);
        nn[i] = minJ;
    }

    // Main linking loop
    size_t level = 0;
    while (level < n - 1) {
        // Calculate minimal distance
        auto minDistanceIt = std::min_element(nnDistances.begin(), nnDistances.end());
        size_t minI = std::distance(nnDistances.begin(), minDistanceIt);
        size_t minJ = nn[minI];
        float minDistance = *minDistanceIt;
        if (minDistance > DistanceThreshold) {
            break;
        }

        // Link minJ to minI
        for (size_t i = 0; i < n; i++) {
            if (Labels[i] == minJ) {
                Labels[i] = minI;
            }
        }

        // Update distance matrix and nearest neighbors
        nnDistances[minI] = INF_DISTANCE;
        for (size_t k = 0; k < static_cast<size_t>(distances.rows()); k++) {
            if (k == minI || k == minJ) {
                continue;
            }
            float newDistance = std::min(distances(minJ, k), distances(minI, k));
            distances(minI, k) = newDistance;
            distances(k, minI) = newDistance;
            if (newDistance < nnDistances[minI]) {
                nnDistances[minI] = newDistance;
                nn[minI] = k;
            }
        }

        // Remove minJ row and column from distance matrix
        nnDistances[minJ] = INF_DISTANCE;
        for (size_t i = 0; i < static_cast<size_t>(distances.rows()); i++) {
            distances(minJ, i) = INF_DISTANCE;
            distances(i, minJ) = INF_DISTANCE;
        }
        level += 1;
    }
}

const std::vector<size_t>& HierarchicalClustering::GetLabels() const {
    return Labels;
}

void HierarchicalClustering::fillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) {
    // Assuming points are on unit sphere
    // Normalize to [0.0, 1.0]
    distances = -((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f;
    distances += distances.Identity(distances.rows(), distances.cols());
}
