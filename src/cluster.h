#pragma once

#include "db_document.h"

#include <functional>

class TAgencyRating;
class TAlexaAgencyRating;

class TNewsCluster {
private:
    uint64_t Id = 0;
    uint64_t FreshestTimestamp = 0;

    uint64_t BestTimestamp = 0;
    double Importance = 0.0;
    std::vector<double> DocWeights;

    std::vector<TDbDocument> Documents;

public:
    TNewsCluster(uint64_t id) : Id(id) {};

    void AddDocument(const TDbDocument& document);
    void Summarize(const TAgencyRating& agencyRating);
    void CalcImportance(const TAlexaAgencyRating& alexaRating);
    bool operator<(const TNewsCluster& other) const;

    uint64_t GetTimestamp(float percentile = 0.9) const;
    tg::ECategory GetCategory() const;

    uint64_t GetFreshestTimestamp() const { return FreshestTimestamp; }
    size_t GetSize() const { return Documents.size(); }
    const std::vector<TDbDocument>& GetDocuments() const { return Documents; }
    std::string GetTitle() const { return Documents.front().Title; }
    std::string GetLanguage() const { return nlohmann::json(Documents.front().Language); }
    double GetImportance() const { return Importance; }
    uint64_t GetBestTimestamp() const { return BestTimestamp; }
    const std::vector<double>& GetDocWeights() const { return DocWeights; }

private:
    void SortByWeights(const std::vector<double>& weights);
};

using TClusters = std::vector<TNewsCluster>;
using TClustersIndex = std::set<TNewsCluster>;
