#include <set>
#include <unordered_map>
#include <cmath>
#include <queue>

#include "rank.h"
#include "../util.h"
#include "../clustering/rank_docs.h"

ENewsCategory ComputeClusterCategory(const TNewsCluster& cluster) {
    std::vector<size_t> categoryCount(NC_COUNT);
    for (const auto& doc : cluster) {
        ENewsCategory docCategory = doc.get().Category;
        assert(docCategory != NC_UNDEFINED && docCategory != NC_NOT_NEWS);
        categoryCount[static_cast<size_t>(docCategory)] += 1;
    }
    auto it = std::max_element(categoryCount.begin(), categoryCount.end());
    return static_cast<ENewsCategory>(std::distance(categoryCount.begin(), it));
}

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t iterTimestamp
) {
    const float clusterTimestampPercentile = 0.9;
    std::set<std::string> seenHosts;
    std::vector<uint64_t> clusterTimestamps;
    double agenciesWeight = 0.0;
    for (const auto& doc : cluster) {
        const std::string& docHost = GetHost(doc.get().Url);
        if (seenHosts.insert(docHost).second) {
            agenciesWeight += ComputeDocAgencyWeight(doc, agencyRating);
        }
        clusterTimestamps.push_back(doc.get().FetchTime);
    }

    std::sort(clusterTimestamps.begin(), clusterTimestamps.end());

    // ~1 for freshest ts, 0.5 for 12 hour old ts, ~0 for 24 hour old ts
    double clusterTimestamp = clusterTimestamps[static_cast<size_t>(std::floor(clusterTimestampPercentile * (clusterTimestamps.size() - 1)))];
    double clusterTimestampRemapped = (clusterTimestamp - iterTimestamp) / 3600.0 + 12;
    double timeMultiplier = Sigmoid(clusterTimestampRemapped);

    // Pessimize only clusters with size < 5
    const size_t clusterSize = cluster.size();
    double smallClusterCoef = std::min(clusterSize * 0.2, 1.0);

    return agenciesWeight * timeMultiplier * smallClusterCoef;
}


std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const std::vector<TNewsCluster>& clusters,
    const std::unordered_map<std::string, double>& agencyRating,
    uint64_t iterTimestamp
) {
    std::vector<TWeightedNewsCluster> weightedClusters;
    for (const TNewsCluster& cluster : clusters) {
        ENewsCategory clusterCategory = ComputeClusterCategory(cluster);
        const std::string& title = cluster[0].get().Title;
        const double weight = ComputeClusterWeight(cluster, agencyRating, iterTimestamp);
        weightedClusters.emplace_back(cluster, clusterCategory, title, weight);
    }

    std::sort(weightedClusters.begin(), weightedClusters.end(),
        [](const TWeightedNewsCluster& a, const TWeightedNewsCluster& b) {
            return a.Weight > b.Weight;
        }
    );

    std::vector<std::vector<TWeightedNewsCluster>> output(NC_COUNT);
    for (const TWeightedNewsCluster& cluster : weightedClusters) {
        assert(cluster.Category != NC_UNDEFINED);
        output[static_cast<size_t>(cluster.Category)].push_back(cluster);
        output[static_cast<size_t>(NC_ANY)].push_back(cluster);
    }

    return output;
}
