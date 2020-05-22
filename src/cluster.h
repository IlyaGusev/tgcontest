#pragma once

#include "db_document.h"

#include <functional>

class TNewsCluster {
private:
    std::vector<std::reference_wrapper<const TDbDocument>> Documents;

public:
    TNewsCluster() = default;

    void AddDocument(const TDbDocument& document);
    uint64_t GetTimestamp(float percentile = 0.9) const;
    uint64_t GetFreshestTimestamp() const;
    tg::ECategory GetCategory() const;
    size_t GetSize() const { return Documents.size(); }
    const std::vector<std::reference_wrapper<const TDbDocument>>& GetDocuments() const { return Documents; }
    std::string GetTitle() const { return Documents.at(0).get().Title; }
    std::string GetLanguage() const { return nlohmann::json(Documents.at(0).get().Language); }
    void SortByWeights(const std::vector<double>& weights);
};

using TClusters = std::vector<TNewsCluster>;
