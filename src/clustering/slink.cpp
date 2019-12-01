#include "slink.h"

#include <string>
#include <vector>


SlinkClustering::SlinkClustering(
    fasttext::FastText& model
    , float distanceThreshold
    , FastTextEmbedder::AggregationMode mode
    , size_t maxWords
    , const std::string& matrixPath
    , const std::string& biasPath
)
    : FastTextEmbedder(model, mode, maxWords, matrixPath, biasPath)
    , DistanceThreshold(distanceThreshold)
{}

// SLINK: https://sites.cs.ucsb.edu/~veronika/MAE/summary_SLINK_Sibson72.pdf
SlinkClustering::Clusters SlinkClustering::Cluster(const std::vector<Document>& docs) {
    const size_t docSize = docs.size();
    const size_t embSize = GetEmbeddingSize();

    Eigen::MatrixXf points(docSize, embSize);
    for (size_t i = 0; i < docSize; i++) {
        fasttext::Vector embedding = GetSentenceEmbedding(docs[i]);
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
    }

    Eigen::MatrixXf distances(points.rows(), points.rows());
    FillDistanceMatrix(points, distances);

    // Prepare 3 arrays
    const float INF_DISTANCE = 1.0f;
    std::vector<size_t> labels(docSize);
    for (size_t i = 0; i < docSize; i++) {
        labels[i] = i;
    }
    std::vector<size_t> nn(docSize);
    std::vector<float> nnDistances(docSize);
    for (size_t i = 0; i < docSize; i++) {
        Eigen::Index minJ;
        nnDistances[i] = distances.row(i).minCoeff(&minJ);
        nn[i] = minJ;
    }

    // Main linking loop
    size_t level = 0;
    while (level + 1 < docSize) {
        // Calculate minimal distance
        auto minDistanceIt = std::min_element(nnDistances.begin(), nnDistances.end());
        size_t minI = std::distance(nnDistances.begin(), minDistanceIt);
        size_t minJ = nn[minI];
        float minDistance = *minDistanceIt;
        if (minDistance > DistanceThreshold) {
            break;
        }

        // Link minJ to minI
        for (size_t i = 0; i < docSize; i++) {
            if (labels[i] == minJ) {
                labels[i] = minI;
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

    SlinkClustering::Clusters clusters(docSize);
    for (size_t i = 0; i < docSize; ++i) {
        const size_t clusterId = labels[i];
        clusters[clusterId].push_back(std::cref(docs[i]));
    }

    return clusters;
}

void SlinkClustering::FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const {
    // Assuming points are on unit sphere
    // Normalize to [0.0, 1.0]
    distances = -((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f;
    distances += distances.Identity(distances.rows(), distances.cols());
}
