#pragma once

#include "db_document.h"
#include "agency_rating.h"

class TAgencyRating;
class TAlexaAgencyRating;

struct TSliceFeatures {
    uint64_t BestTimestamp = 0;
    double Importance = 0.0;
    std::vector<double> DocWeights;
    std::map<std::string, double> CountryShare;
    std::map<std::string, double> WeightedCountryShare;
};


class TNewsCluster {
private:
    uint64_t Id = 0;
    uint64_t FreshestTimestamp = 0;

    tg::ECategory Category = tg::NC_UNDEFINED;
    std::vector<TDbDocument> Documents;

public:
    explicit TNewsCluster(uint64_t id) : Id(id) {};

    void AddDocument(const TDbDocument& document);
    void CalcCategory();

    bool operator<(const TNewsCluster& other) const;
    static bool Compare(const TNewsCluster& cluster, uint64_t timestamp);

    uint64_t GetTimestamp(float percentile = 0.9) const;

    tg::ECategory GetCategory() const { return Category; }
    uint64_t GetFreshestTimestamp() const { return FreshestTimestamp; }
    size_t GetSize() const { return Documents.size(); }
    const std::vector<TDbDocument>& GetDocuments() const { return Documents; }
    std::string GetTitle() const { return Documents.front().Title; }
    tg::ELanguage GetLanguage() const { return Documents.front().Language; }
private:
    void SortByWeights(const std::vector<double>& weights);
};

using TClusters = std::vector<TNewsCluster>;
