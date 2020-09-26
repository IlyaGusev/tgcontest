#pragma once

#include "agency_rating.h"
#include "cluster.h"
#include "config.pb.h"

struct TSliceFeatures {
    uint64_t BestTimestamp = 0;
    double Importance = 0.0;
    std::vector<double> DocWeights;
    std::map<std::string, double> CountryShare;
    std::map<std::string, double> WeightedCountryShare;
};

class TSummarizer {
public:
    TSummarizer(const std::string& configPath);

    void Summarize(TClusters& clusters) const;

    static constexpr std::array<const char*, 6> RegionCodes;
    static constexpr std::array<double, 4> Decays;
    static constexpr std::array<double, 3> Shifts;

private:
    void RankDocuments(TNewsCluster& cluster, const TAgencyRating& agencyRating);

private:
    tg::TSummarizerConfig Config;
    TAgencyRating AgencyRating;
    TAlexaAgencyRating AlexaAgencyRating;
};
