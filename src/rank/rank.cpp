#include <set>
#include <unordered_map>
#include <cmath>
#include <queue>

#include "rank.h"
#include "../util.h"
#include "../clustering/rank_docs.h"

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t iterTimestamp
) {
    std::set<std::string> seenHosts;
    double agenciesWeight = 0.0;
    for (const TDocument& doc : cluster.GetDocuments()) {
        const std::string& docHost = GetHost(doc.Url);
        if (seenHosts.insert(docHost).second) {
            agenciesWeight += ComputeDocAgencyWeight(doc, agencyRating);
        }
    }

    // ~1 for freshest ts, 0.5 for 12 hour old ts, ~0 for 24 hour old ts
    double clusterTimestampRemapped = (cluster.GetTimestamp() - iterTimestamp) / 3600.0 + 12;
    double timeMultiplier = Sigmoid(clusterTimestampRemapped);

    // Pessimize only clusters with size < 5
    const size_t clusterSize = cluster.GetSize();
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
        ENewsCategory clusterCategory = cluster.GetCategory();
        const std::string& title = cluster.GetTitle();
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
