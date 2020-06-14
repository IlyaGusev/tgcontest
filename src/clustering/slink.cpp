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
using namespace torch::indexing;

constexpr float INF_DISTANCE = 1.0f;

void ApplyTimePenalty(
    const std::vector<TDbDocument>::const_iterator begin,
    size_t docSize,
    torch::Tensor& distances
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
            distances[i][j] = std::min(penalty * distances[i][j].item<float>(), INF_DISTANCE);
            distances[j][i] = distances[i][j];
        }
    }
}

bool IsNewClusterSizeAcceptable(size_t newClusterSize, float newDistance, const tg::TClusteringConfig& config) {
    if (newClusterSize <= config.small_cluster_size()) {
        return true;
    } else if (newClusterSize <= config.medium_cluster_size()) {
        return newDistance <= config.medium_threshold();
    } else if (newClusterSize <= config.large_cluster_size()) {
        return newDistance <= config.large_threshold();
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

TSlinkClustering::TSlinkClustering(const tg::TClusteringConfig& config)
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
        size_t batchSize = std::min(remainingDocsCount, static_cast<size_t>(Config.chunk_size()));
        std::vector<TDbDocument>::const_iterator end = begin + batchSize;

        std::vector<size_t> newLabels = ClusterBatch(begin, end, embeddingKey);
        size_t newMaxLabel = maxLabel;
        for (auto& label : newLabels) {
            label += maxLabel;
            newMaxLabel = std::max(newMaxLabel, label);
        }
        maxLabel = newMaxLabel;

        assert(begin->Url == docs[batchStart].Url);
        for (size_t i = batchStart; i < batchStart + Config.intersection_size() && i < labels.size(); i++) {
            size_t oldLabel = labels[i];
            int j = i - batchStart;
            assert(j >= 0 && static_cast<size_t>(j) < newLabels.size());
            size_t newLabel = newLabels[j];
            oldLabelsToNew[oldLabel] = newLabel;
        }
        if (batchStart == 0) {
            for (size_t i = 0; i < std::min(static_cast<size_t>(Config.intersection_size()), newLabels.size()); i++) {
                labels.push_back(newLabels[i]);
            }
        }
        for (size_t i = Config.intersection_size(); i < newLabels.size(); i++) {
            labels.push_back(newLabels[i]);
        }
        assert(batchStart == static_cast<size_t>(std::distance(docs.begin(), begin)));
        assert(batchStart + batchSize == static_cast<size_t>(std::distance(docs.begin(), end)));
        for (const auto& pair : oldLabelsToNew) {
            assert(pair.first < pair.second);
        }

        prevBatchEnd = batchStart + batchSize;
        batchStart = batchStart + batchSize - Config.intersection_size();
        begin = end - Config.intersection_size();
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
            clusters.emplace_back(newLabel);
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
    const size_t embSize = begin->Embeddings.at(embeddingKey).size();
    assert(docSize != 0 && embSize != 0);

    auto pointsSizes = {static_cast<long int>(docSize), static_cast<long int>(embSize)};
    auto distancesSizes = {static_cast<long int>(docSize), static_cast<long int>(docSize)};
    auto embSizes = {static_cast<long int>(embSize)};

    auto points = torch::zeros(pointsSizes, torch::requires_grad(false));
    std::vector<TDbDocument>::const_iterator docsIt = begin;
    for (size_t i = 0; i < docSize; ++i) {
        const std::vector<float>& embedding = docsIt->Embeddings.at(embeddingKey);
        // UNSAFE: undefined behavior, const_cast on originally const data
        points[i] = torch::from_blob(const_cast<float*>(embedding.data()), embSizes);
        docsIt++;
    }

    auto distances = torch::zeros(distancesSizes, torch::requires_grad(false));
    FillDistanceMatrix(points, distances);

    if (Config.use_timestamp_moving()) {
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
        size_t minJ = static_cast<size_t>(distances[i].argmin().item<long int>());
        nnDistances[i] = distances[i][minJ].item<float>();
        nn[i] = minJ;
    }

    std::vector<size_t> clusterSizes(docSize);
    std::vector<TClusterSiteNames> clusterSiteNames(docSize);

    for (size_t i = 0; i < docSize; i++) {
        clusterSizes[i] = 1;

        if (Config.ban_same_hosts()) {
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

        TClusterSiteNames* firstClusterSiteNames = Config.ban_same_hosts() ? &clusterSiteNames[minI] : nullptr;
        TClusterSiteNames* secondClusterSiteNames = Config.ban_same_hosts() ? &clusterSiteNames[minJ] : nullptr;

        if (minDistance > Config.small_threshold()) {
            break;
        }

        const size_t newClusterSize = firstClusterSize + secondClusterSize;
        if (!IsNewClusterSizeAcceptable(newClusterSize, minDistance, Config)
            || (Config.ban_same_hosts() && HasSameSource(*firstClusterSiteNames, *secondClusterSiteNames))
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
        if (Config.ban_same_hosts()) {
            firstClusterSiteNames->insert(secondClusterSiteNames->begin(), secondClusterSiteNames->end());
        }

        // Update distance matrix and nearest neighbors
        nnDistances[minI] = INF_DISTANCE;
        for (size_t k = 0; k < docSize; k++) {
            if (k == minI || k == minJ) {
                continue;
            }
            float newDistance = std::min(distances[minJ][k].item<float>(), distances[minI][k].item<float>());
            distances[minI][k] = newDistance;
            distances[k][minI] = newDistance;
            if (newDistance < nnDistances[minI]) {
                nnDistances[minI] = newDistance;
                nn[minI] = k;
            }
        }

        // Remove minJ row and column from distance matrix
        nnDistances[minJ] = INF_DISTANCE;
        for (size_t i = 0; i < docSize; i++) {
            distances[minJ][i] = INF_DISTANCE;
            distances[i][minJ] = INF_DISTANCE;
        }
    }

    return labels;
}

void TSlinkClustering::FillDistanceMatrix(const torch::Tensor& points, torch::Tensor& distances) const {
    // Assuming points are on unit sphere
    // Normalize to [0.0, 1.0]
    distances = -((torch::matmul(points, points.transpose(0, 1))) + 1.0f) / 2.0f + 1.0f;
    distances += torch::eye(distances.size(0), distances.size(1));
}
