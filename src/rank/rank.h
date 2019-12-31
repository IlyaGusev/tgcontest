#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include "../clustering/clustering.h"


struct TWeightedNewsCluster {
    std::reference_wrapper<const TNewsCluster> Cluster;
    ENewsCategory Category;
    std::string Title;
    double Weight = 0.0;
    TWeightedNewsCluster(const TNewsCluster& cluster, ENewsCategory category, const std::string& title, double weight)
        : Cluster(cluster)
        , Category(category)
        , Title(title)
        , Weight(weight)
    {}
};

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t iterTimestamp
);

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const std::vector<TNewsCluster>& clusters,
    const std::unordered_map<std::string, double>& agencyRating,
    uint64_t iterTimestamp
);
