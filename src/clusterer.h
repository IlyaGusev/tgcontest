#pragma once

#include "agency_rating.h"
#include "cluster.h"
#include "clustering/clustering.h"
#include "config.pb.h"
#include "db_document.h"

#include <vector>
#include <memory>

struct TClusterIndex {
    std::unordered_map<tg::ELanguage, TClusters> Clusters;
    uint64_t IterTimestamp = 0;
    uint64_t TrueMaxTimestamp = 0;
};

class TClusterer {
public:
    TClusterer(const std::string& configPath);

    TClusterIndex Cluster(std::vector<TDbDocument>&& docs) const;

private:
    void Summarize(TClusters& clusters) const;
    void CalcWeights(TClusters& clusters) const;

private:
    tg::TClustererConfig Config;
    std::unordered_map<tg::ELanguage, std::unique_ptr<TClustering>> Clusterings;
};
