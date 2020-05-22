#include "agency_rating.h"
#include "clustering/clustering.h"
#include "db_document.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>


struct TWeightedNewsCluster {
    std::reference_wrapper<const TNewsCluster> Cluster;
    tg::ECategory Category;
    std::string Title;
    double Weight = 0.0;
    TWeightedNewsCluster(const TNewsCluster& cluster, tg::ECategory category, const std::string& title, double weight)
        : Cluster(cluster)
        , Category(category)
        , Title(title)
        , Weight(weight)
    {}
};

double ComputeClusterWeight(
    const TNewsCluster& cluster,
    const TAgencyRating& agencyRating,
    const uint64_t iterTimestamp
);

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    const TClusters& clusters,
    const TAgencyRating& agencyRating,
    uint64_t iterTimestamp
);
