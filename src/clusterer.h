#pragma once

#include "agency_rating.h"
#include "cluster.h"
#include "clustering/clustering.h"
#include "config.pb.h"
#include "db_document.h"

#include <vector>
#include <memory>

class TClusterer {
public:
    TClusterer(const std::string& configPath);

    std::unordered_map<tg::ELanguage, TClusters> Cluster(
        std::vector<TDbDocument>& docs,
        uint64_t& iterTimestamp
    ) const;

private:
    void Summarize(TClusters& clusters) const;
    void CalcWeights(TClusters& clusters) const;
    void ParseConfig(const std::string& fname);

private:
    tg::TClustererConfig Config;
    std::unordered_map<tg::ELanguage, std::unique_ptr<TClustering>> Clusterings;
    TAgencyRating AgencyRating;
    TAlexaAgencyRating AlexaAgencyRating;
};
