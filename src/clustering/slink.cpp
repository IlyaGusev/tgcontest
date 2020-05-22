#include "slink.h"
#include "../util.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using TClusterSiteNames = std::unordered_set<std::string>;

constexpr float INF_DISTANCE = 1.0f;

void ApplyTimePenalty(
    const std::vector<TDbDocument>::const_iterator begin,
    size_t docSize,
    Eigen::MatrixXf& distances
) {
    std::vector<TDbDocument>::const_iterator iIt = begin;
    std::vector<TDbDocument>::const_iterator jIt = begin + 1;
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

bool IsNewClusterSizeAcceptable(size_t newClusterSize, float newDistance, const TSlinkClustering::TConfig& config) {
    if (newClusterSize <= config.SmallClusterSize) {
        return true;
    } else if (newClusterSize <= config.MediumClusterSize) {
        return newDistance <= config.MediumClusterThreshold;
    } else if (newClusterSize <= config.LargeClusterSize) {
        return newDistance <= config.LargeClusterThreshold;
    }
    return false;
}

bool CheckSetIntersection(const TClusterSiteNames& smallerSet, const TClusterSiteNames& largerSet) {
    return std::any_of(smallerSet.begin(), smallerSet.end(), [&largerSet](const auto& siteName) {
        return largerSet.find(siteName) != largerSet.end();
    });
}

bool HasSameSource(const TClusterSiteNames& firstSet, const TClusterSiteNames& secondSet) {
    if (firstSet.size() < secondSet.size()) {
        return CheckSetIntersection(firstSet, secondSet);
    }
    return CheckSetIntersection(secondSet, firstSet);
}

} // namespace

TSlinkClustering::TSlinkClustering(const TConfig& config)
    : Config(config)
{
}

TClusters TSlinkClustering::Cluster(
    const std::vector<TDbDocument>& docs,
    tg::EEmbeddingKey embeddingKey
) {
    const size_t docSize = docs.size();
    std::vector<size_t> labels;
    labels.reserve(docSize);

    std::vector<TDbDocument>::const_iterator begin = docs.cbegin();
    std::unordered_map<size_t, size_t> oldLabelsToNew;
    size_t batchStart = 0;
    size_t prevBatchEnd = batchStart;
    size_t maxLabel = 0;
    while (prevBatchEnd < docs.size()) {
        size_t remainingDocsCount = docSize - batchStart;
        size_t batchSize = std::min(remainingDocsCount, Config.BatchSize);
        std::vector<TDbDocument>::const_iterator end = begin + batchSize;

        std::vector<size_t> newLabels = ClusterBatch(begin, end, embeddingKey);
        size_t newMaxLabel = maxLabel;
        for (auto& label : newLabels) {
            label += maxLabel;
            newMaxLabel = std::max(newMaxLabel, label);
        }
        maxLabel = newMaxLabel;

        assert(begin->Url == docs[batchStart].Url);
        for (size_t i = batchStart; i < batchStart + Config.BatchIntersectionSize && i < labels.size(); i++) {
            size_t oldLabel = labels[i];
            int j = i - batchStart;
            assert(j >= 0 && static_cast<size_t>(j) < newLabels.size());
            size_t newLabel = newLabels[j];
            oldLabelsToNew[oldLabel] = newLabel;
        }
        if (batchStart == 0) {
            for (size_t i = 0; i < std::min(Config.BatchIntersectionSize, newLabels.size()); i++) {
                labels.push_back(newLabels[i]);
            }
        }
        for (size_t i = Config.BatchIntersectionSize; i < newLabels.size(); i++) {
            labels.push_back(newLabels[i]);
        }
        assert(batchStart == static_cast<size_t>(std::distance(docs.begin(), begin)));
        assert(batchStart + batchSize == static_cast<size_t>(std::distance(docs.begin(), end)));
        for (const auto& pair : oldLabelsToNew) {
            assert(pair.first < pair.second);
        }

        prevBatchEnd = batchStart + batchSize;
        batchStart = batchStart + batchSize - Config.BatchIntersectionSize;
        begin = end - Config.BatchIntersectionSize;
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
    TClusters clusters;
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
    const std::vector<TDbDocument>::const_iterator begin,
    const std::vector<TDbDocument>::const_iterator end,
    tg::EEmbeddingKey embeddingKey
) {
    const size_t docSize = std::distance(begin, end);
    assert(docSize != 0);
    const size_t embSize = begin->Embeddings.at(embeddingKey).size();

    Eigen::MatrixXf points(docSize, embSize);
    std::vector<TDbDocument>::const_iterator docsIt = begin;
    for (size_t i = 0; i < docSize; ++i) {
        std::vector<float> embedding = docsIt->Embeddings.at(embeddingKey);
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
        docsIt++;
    }

    Eigen::MatrixXf distances(points.rows(), points.rows());
    FillDistanceMatrix(points, distances);

    if (Config.UseTimestampMoving) {
        ApplyTimePenalty(begin, docSize, distances);
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

    std::vector<size_t> clusterSizes(docSize);
    std::vector<TClusterSiteNames> clusterSiteNames(docSize);

    for (size_t i = 0; i < docSize; i++) {
        clusterSizes[i] = 1;

        if (Config.BanThreadsFromSameSite) {
            clusterSiteNames[i].insert((begin + i)->SiteName);
        }
    }

    // Main linking loop
    for (size_t level = 0; level + 1 < docSize; ++level) {
        // Calculate minimal distance
        auto minDistanceIt = std::min_element(nnDistances.begin(), nnDistances.end());
        size_t minI = std::distance(nnDistances.begin(), minDistanceIt);
        size_t minJ = nn[minI];
        float minDistance = *minDistanceIt;

        const size_t firstClusterSize = clusterSizes[minI];
        const size_t secondClusterSize = clusterSizes[minJ];

        TClusterSiteNames* firstClusterSiteNames = Config.BanThreadsFromSameSite ? &clusterSiteNames[minI] : nullptr;
        TClusterSiteNames* secondClusterSiteNames = Config.BanThreadsFromSameSite ? &clusterSiteNames[minJ] : nullptr;

        if (minDistance > Config.SmallClusterThreshold) {
            break;
        }

        const size_t newClusterSize = firstClusterSize + secondClusterSize;
        if (!IsNewClusterSizeAcceptable(newClusterSize, minDistance, Config)
            || (Config.BanThreadsFromSameSite && HasSameSource(*firstClusterSiteNames, *secondClusterSiteNames))
        ) {
            nnDistances[minI] = INF_DISTANCE;
            nnDistances[minJ] = INF_DISTANCE;
            continue;
        }

        // Link minJ to minI
        for (size_t i = 0; i < docSize; i++) {
            if (labels[i] == minJ) {
                labels[i] = minI;
            }
        }

        clusterSizes[minI] = newClusterSize;
        if (Config.BanThreadsFromSameSite) {
            firstClusterSiteNames->insert(secondClusterSiteNames->begin(), secondClusterSiteNames->end());
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
    }

    return labels;
}

void TSlinkClustering::FillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances) const {
    // Assuming points are on unit sphere
    // Normalize to [0.0, 1.0]
    distances = -((points * points.transpose()).array() + 1.0f) / 2.0f + 1.0f;
    distances += distances.Identity(distances.rows(), distances.cols());
}
