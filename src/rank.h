#include "agency_rating.h"
#include "clustering/clustering.h"
#include "db_document.h"

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
    TWeightInfo WeightInfo;
    TWeightedNewsCluster(const TNewsCluster& cluster, const TWeightInfo& info)
        : Cluster(cluster)
        , WeightInfo(info)
    {}
};

std::vector<std::vector<TWeightedNewsCluster>> Rank(
    TClusters::const_iterator begin,
    TClusters::const_iterator end,
    uint64_t iterTimestamp,
    uint64_t window
);
