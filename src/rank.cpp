#include "rank.h"
#include "util.h"

TWeightInfo ComputeClusterWeightPush(
    const TNewsCluster& cluster,
    const uint64_t iterTimestamp,
    const uint64_t window
) {
    double timeMultiplier = 1.;

    // ~1 for freshest ts, 0.5 for 12 hour old ts, ~0 for 24 hour old ts
    int32_t clusterTime = cluster.GetBestTimestamp();
    if (clusterTime + window < iterTimestamp) {
        double clusterTimestampRemapped = static_cast<double>(clusterTime + static_cast<int32_t>(window) - static_cast<int32_t>(iterTimestamp)) / 3600.0 + 12.0;
        timeMultiplier = Sigmoid(clusterTimestampRemapped);
    }

    double rank = cluster.GetImportance();
    return TWeightInfo{clusterTime, rank, timeMultiplier, rank * timeMultiplier, cluster.GetSize()};
}

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    TClusters::const_iterator begin,
    TClusters::const_iterator end,
    uint64_t iterTimestamp,
    uint64_t window
) {
    std::vector<TWeightedNewsCluster> weightedClusters;
    for (TClusters::const_iterator it = begin; it != end; it++) {
        const TNewsCluster& cluster = *it;
        const TWeightInfo weight = ComputeClusterWeightPush(cluster, iterTimestamp, window);
        weightedClusters.emplace_back(cluster, std::move(weight));
    }

    std::stable_sort(weightedClusters.begin(), weightedClusters.end(),
        [](const TWeightedNewsCluster& a, const TWeightedNewsCluster& b) {
            if (a.WeightInfo.ClusterSize == b.WeightInfo.ClusterSize) {
                return a.WeightInfo.Weight > b.WeightInfo.Weight;
            }
            if (a.WeightInfo.ClusterSize < 3 || b.WeightInfo.ClusterSize < 3) {
                return a.WeightInfo.ClusterSize > b.WeightInfo.ClusterSize;
            }
            return a.WeightInfo.Weight > b.WeightInfo.Weight;
        }
    );

    std::vector<std::vector<TWeightedNewsCluster>> output(tg::ECategory_ARRAYSIZE);
    for (const TWeightedNewsCluster& cluster : weightedClusters) {
        auto category = cluster.Cluster.get().GetCategory();
        assert(category != tg::NC_UNDEFINED && category != tg::NC_ANY);
        output[static_cast<size_t>(category)].push_back(cluster);
        output[static_cast<size_t>(tg::NC_ANY)].push_back(cluster);
    }

    return output;
}
