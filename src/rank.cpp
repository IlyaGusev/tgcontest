#include "rank.h"
#include "util.h"

TWeightInfo ComputeClusterWeightPush(
    const TNewsCluster& cluster,
    const uint64_t iterTimestamp,
    const uint64_t window
) {
    double timeMultiplier = 1.;

    // ~1 for freshest ts, 0.5 for 8 hour old ts, ~0 for 16 hour old ts
    int32_t clusterTime = cluster.GetBestTimestamp();
    if (clusterTime + window < iterTimestamp) {
        double clusterTimestampRemapped = static_cast<double>(clusterTime + static_cast<int32_t>(window) - static_cast<int32_t>(iterTimestamp)) / 3600.0 + 8.0;
        timeMultiplier = Sigmoid(clusterTimestampRemapped);
    }

    double rank = cluster.GetImportance();
    TWeightInfo info{clusterTime, rank, timeMultiplier, rank * timeMultiplier};
    return info;
}

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const TClusters& clusters,
    uint64_t iterTimestamp,
    uint64_t window
) {
    std::vector<TWeightedNewsCluster> weightedClusters;
    for (const TNewsCluster& cluster : clusters) {
        tg::ECategory clusterCategory = cluster.GetCategory();
        const std::string& title = cluster.GetTitle();
        const TWeightInfo weight = ComputeClusterWeightPush(cluster, iterTimestamp, window);
        weightedClusters.emplace_back(cluster, clusterCategory, title, weight, cluster.GetDocWeights());
    }

    std::stable_sort(weightedClusters.begin(), weightedClusters.end(),
        [](const TWeightedNewsCluster& a, const TWeightedNewsCluster& b) {
            return a.WeightInfo.Weight > b.WeightInfo.Weight;
        }
    );

    std::vector<std::vector<TWeightedNewsCluster>> output(tg::ECategory_ARRAYSIZE);
    for (const TWeightedNewsCluster& cluster : weightedClusters) {
        assert(cluster.Category != tg::NC_UNDEFINED);
        output[static_cast<size_t>(cluster.Category)].push_back(cluster);
        output[static_cast<size_t>(tg::NC_ANY)].push_back(cluster);
    }

    return output;
}
