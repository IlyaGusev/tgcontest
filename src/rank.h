#include "agency_rating.h"
#include "clustering/clustering.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

struct TWeightInfo {
    int32_t BestTime = 0;
    double Importance = 0.;
    double AgePenalty = 1.;
    double Weight = 0.;
};


struct TWeightedNewsCluster {
    std::reference_wrapper<const TNewsCluster> Cluster;
    ENewsCategory Category;
    std::string Title;
    TWeightInfo WeightInfo;
    std::vector<double> DocWeights;
    TWeightedNewsCluster(const TNewsCluster& cluster, ENewsCategory category, const std::string& title, const TWeightInfo& info, const std::vector<double>& docWeights)
        : Cluster(cluster)
        , Category(category)
        , Title(title)
        , WeightInfo(info)
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
    const TAlexaAgencyRating& alexaAgencyRating,
    uint64_t iterTimestamp,
    uint64_t window
);
