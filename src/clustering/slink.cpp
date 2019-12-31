#include "slink.h"
#include "../util.h"

#include <string>
#include <vector>

TSlinkClustering::TSlinkClustering(
    TFastTextEmbedder& embedder
    , float distanceThreshold
    , size_t batchSize
    , size_t batchIntersectionSize
    , bool useTimestampMoving
)
    : TClustering(embedder)
    , DistanceThreshold(distanceThreshold)
    , BatchSize(batchSize)
    , BatchIntersectionSize(batchIntersectionSize)
    , UseTimestampMoving(useTimestampMoving)
{}

TSlinkClustering::TClusters TSlinkClustering::Cluster(
    const std::vector<TDocument>& docs
) {
    const size_t docSize = docs.size();
    std::vector<size_t> labels;
    labels.reserve(docSize);

    std::vector<TDocument>::const_iterator begin = docs.cbegin();
    std::unordered_map<size_t, size_t> oldLabelsToNew;
    size_t batchStart = 0;
    size_t prevBatchEnd = batchStart;
    size_t maxLabel = 0;
    while (prevBatchEnd < docs.size()) {
        size_t remainingDocsCount = docSize - batchStart;
        size_t batchSize = std::min(remainingDocsCount, BatchSize);
        std::vector<TDocument>::const_iterator end = begin + batchSize;

        std::vector<size_t> newLabels = ClusterBatch(begin, end);
        size_t newMaxLabel = maxLabel;
        for (auto& label : newLabels) {
            label += maxLabel;
            newMaxLabel = std::max(newMaxLabel, label);
        }
        maxLabel = newMaxLabel;

        assert(begin->Url == docs[batchStart].Url);
        for (size_t i = batchStart; i < batchStart + BatchIntersectionSize && i < labels.size(); i++) {
            size_t oldLabel = labels[i];
            int j = i - batchStart;
            assert(j >= 0 && j < newLabels.size());
            size_t newLabel = newLabels[j];
            oldLabelsToNew[oldLabel] = newLabel;
        }
        if (batchStart == 0) {
            for (size_t i = 0; i < std::min(BatchIntersectionSize, newLabels.size()); i++) {
                labels.push_back(newLabels[i]);
            }
        }
        for (size_t i = BatchIntersectionSize; i < newLabels.size(); i++) {
            labels.push_back(newLabels[i]);
        }
        assert(batchStart == static_cast<size_t>(std::distance(docs.begin(), begin)));
        assert(batchStart + batchSize == static_cast<size_t>(std::distance(docs.begin(), end)));
        for (const auto& pair : oldLabelsToNew) {
            assert(pair.first < pair.second);
        }

        prevBatchEnd = batchStart + batchSize;
        batchStart = batchStart + batchSize - BatchIntersectionSize;
        begin = end - BatchIntersectionSize;
    }
    assert(labels.size() == docs.size());
    for (auto& label : labels) {
        auto it = oldLabelsToNew.find(label);
        if (it == oldLabelsToNew.end()) {
            continue;
        }
        label = it->second;
    }

    std::unordered_map<size_t, size_t> clusterLabels;
    TSlinkClustering::TClusters clusters;
    for (size_t i = 0; i < docSize; ++i) {
        const size_t clusterId = labels[i];
        auto it = clusterLabels.find(clusterId);
        if (it == clusterLabels.end()) {
            size_t newLabel = clusters.size();
            clusterLabels[clusterId] = newLabel;
            clusters.push_back(TNewsCluster());
            clusters[newLabel].AddDocument(docs[i]);
        } else {
            clusters[it->second].AddDocument(docs[i]);
        }
    }
    return clusters;
}

// SLINK: https://sites.cs.ucsb.edu/~veronika/MAE/summary_SLINK_Sibson72.pdf
std::vector<size_t> TSlinkClustering::ClusterBatch(
    const std::vector<TDocument>::const_iterator begin,
    const std::vector<TDocument>::const_iterator end
) {
    const size_t docSize = std::distance(begin, end);
    const size_t embSize = Embedder.GetEmbeddingSize();

    Eigen::MatrixXf points(docSize, embSize);
    std::vector<TDocument>::const_iterator docsIt = begin;
    for (size_t i = 0; i < docSize; ++i) {
        fasttext::Vector embedding = Embedder.GetSentenceEmbedding(*docsIt);
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
        docsIt++;
    }

    Eigen::MatrixXf distances(points.rows(), points.rows());
    FillDistanceMatrix(points, distances);
    const float INF_DISTANCE = 1.0f;

    if (UseTimestampMoving) {
        std::vector<TDocument>::const_iterator iIt = begin;
        std::vector<TDocument>::const_iterator jIt = begin + 1;
        for (size_t i = 0; i < docSize; ++i, ++iIt) {
            jIt = iIt + 1;
            for (size_t j = i + 1; j < docSize; ++j, ++jIt) {
                uint64_t leftTs = iIt->FetchTime;
                uint64_t rightTs = jIt->FetchTime;
                uint64_t diff = rightTs > leftTs ? rightTs - leftTs : leftTs - rightTs;
                float diffHours = static_cast<float>(diff) / 3600.0f;
                float penalty = 1.0f;
                if (diffHours >= 24.0f) {
                    penalty = diffHours / 24.0f;
                }
                distances(i, j) = std::min(penalty * distances(i, j), INF_DISTANCE);
                distances(j, i) = distances(i, j);
            }
        }
    }

    // Prepare 3 arrays
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

    return labels;
}

void TSlinkClustering::FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const {
    // Assuming points are on unit sphere
    // Normalize to [0.0, 1.0]
    distances = -((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f;
    distances += distances.Identity(distances.rows(), distances.cols());
}
