#include "agency_rating.h"
#include "clustering/clustering.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>


struct TWeightedNewsCluster {
    std::reference_wrapper<const TNewsCluster> Cluster;
    ENewsCategory Category;
    std::string Title;
    double Weight = 0.0;
    std::vector<double> DocWeights;
    TWeightedNewsCluster(const TNewsCluster& cluster, ENewsCategory category, const std::string& title, double weight, std::vector<double> docWeights)
        : Cluster(cluster)
        , Category(category)
        , Title(title)
        , Weight(weight)
        , DocWeights(docWeights)
    {}
};

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const TAgencyRating& agencyRating,
    const uint64_t iterTimestamp,
    std::vector<double>& docWeights
);

double ComputeClusterWeightNew(
    const TNewsCluster& cluster,
    const TAgencyRating& agencyRating,
    const uint64_t iterTimestamp,
    std::vector<double>& docWeights
);

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const TClusters& clusters,
    const TAgencyRating& agencyRating,
    uint64_t iterTimestamp,
    uint64_t window
);
