#include <set>
#include <unordered_map>
#include <cmath>
#include <queue>

#include "rank.h"
#include "../util.h"
#include "../clustering/rank_docs.h"

static const char* MAIN_CATEGORY = "any";

uint64_t GetIterTimestamp(const std::vector<TNewsCluster>& clusters, double percentile) {
    // In production ts.now() should be here.
    // In this case we have percentile of documents timestamps because of the small percent of wrong dates.
    std::priority_queue<uint64_t, std::vector<uint64_t>, std::greater<uint64_t>> timestamps;
    uint64_t numDocs = 0;
    for (const auto& cluster: clusters) {
        numDocs += cluster.size();
    }
    size_t prioritySize = numDocs - std::floor(percentile * numDocs);

    for (const auto& cluster : clusters) {
        for (const auto& doc: cluster) {
            timestamps.push(doc.get().FetchTime);
            if (timestamps.size() > prioritySize) {
                timestamps.pop();
            }
        }
    }

    return timestamps.size() > 0 ? timestamps.top() : 0;
}

std::string ComputeClusterCategory(const TNewsCluster& cluster) {
    std::unordered_map<std::string, size_t> categoryCount;
    for (const auto& doc : cluster) {
        std::string docCategory = doc.get().Category;
        categoryCount[docCategory] += 1;
    }
    const std::string& clusterCategory = std::max_element(categoryCount.begin(), categoryCount.end(),
        [](const std::pair<std::string, size_t>& a, const std::pair<std::string, size_t>& b) {
            return a.second < b.second;
        }
    )->first;

    return clusterCategory;
}

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t iterTimestamp
) {
    double output = 0.0;
    const float clusterTimestampPercentile = 0.9;
    std::set<std::string> seenHosts;

    std::vector<uint64_t> clusterTimestamps;

    for (const auto& doc : cluster) {
        const std::string& docHost = GetHost(doc.get().Url);
        if (seenHosts.insert(docHost).second) {
            output += ComputeDocAgencyWeight(doc, agencyRating);
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

    return output * timeMultiplier * smallClusterCoef;
}


std::unordered_map<std::string, std::vector<TWeightedNewsCluster>> Rank(
    const std::vector<TNewsCluster>& clusters,
    const std::unordered_map<std::string, double>& agencyRating,
    uint64_t iterTimestamp
) {
    std::vector<TWeightedNewsCluster> weightedClusters;
    for (const TNewsCluster& cluster : clusters) {
        const std::string clusterCategory = ComputeClusterCategory(cluster);
        const std::string& title = cluster[0].get().Title;
        const double weight = ComputeClusterWeight(cluster, agencyRating, iterTimestamp);
        weightedClusters.emplace_back(cluster, clusterCategory, title, weight);
    }

    std::sort(weightedClusters.begin(), weightedClusters.end(),
        [](const TWeightedNewsCluster& a, const TWeightedNewsCluster& b) {
            return a.Weight > b.Weight;
        }
    );

    std::unordered_map<std::string, std::vector<TWeightedNewsCluster>> output;
    for (const TWeightedNewsCluster& cluster : weightedClusters) {
        output[cluster.Category].push_back(cluster);
        output[MAIN_CATEGORY].push_back(cluster);
    }

    return output;
}
