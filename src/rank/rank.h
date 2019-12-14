#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include "../clustering/clustering.h"


struct TWeightedNewsCluster {
    std::reference_wrapper<const TNewsCluster> Cluster;
    std::string Category;
    std::string Title;
    double Weight = 0.0;
    TWeightedNewsCluster(const TNewsCluster& cluster, const std::string& category, const std::string& title, double weight)
        : Cluster(cluster)
        , Category(category)
        , Title(title)
        , Weight(weight)
    {}

};

uint64_t GetIterTimestamp(const std::vector<TNewsCluster>&); // the latest document timestamp

std::string ComputeClusterCategory(const TNewsCluster& cluster);

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t iterTimestamp
);

std::unordered_map<std::string, std::vector<TWeightedNewsCluster>> Rank(
    const std::vector<TNewsCluster>& clusters,
    const std::unordered_map<std::string, double>& agencyRating
);
