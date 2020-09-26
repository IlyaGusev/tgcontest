#include "cluster.h"

#include "agency_rating.h"
#include "util.h"

#include <boost/range/algorithm/nth_element.hpp>
#include <Eigen/Core>

#include <cassert>
#include <cmath>
#include <set>
#include <vector>

void TNewsCluster::AddDocument(const TDbDocument& document) {
    Documents.push_back(std::move(document));
    FreshestTimestamp = std::max(FreshestTimestamp, static_cast<uint64_t>(Documents.back().FetchTime));
}

uint64_t TNewsCluster::GetTimestamp(float percentile) const {
    assert(!Documents.empty());
    std::vector<uint64_t> clusterTimestamps;
    clusterTimestamps.reserve(Documents.size());
    for (const TDbDocument& doc : Documents) {
        clusterTimestamps.push_back(doc.FetchTime);
    }
    size_t index = static_cast<size_t>(std::floor(percentile * (clusterTimestamps.size() - 1)));
    boost::range::nth_element(clusterTimestamps, clusterTimestamps.begin() + index);
    return clusterTimestamps[index];
}

void TNewsCluster::CalcCategory() {
    std::vector<size_t> categoryCount(tg::ECategory_ARRAYSIZE);
    for (const TDbDocument& doc : Documents) {
        tg::ECategory docCategory = doc.Category;
        assert(doc.IsNews());
        categoryCount[static_cast<size_t>(docCategory)] += 1;
    }
    auto it = std::max_element(categoryCount.begin(), categoryCount.end());
    Category = static_cast<tg::ECategory>(std::distance(categoryCount.begin(), it));
}

void TNewsCluster::SortByWeights(const std::vector<double>& weights) {
    std::vector<std::pair<TDbDocument, double>> weightedDocs;
    weightedDocs.reserve(Documents.size());
    for (size_t i = 0; i < Documents.size(); i++) {
        weightedDocs.emplace_back(std::move(Documents[i]), weights[i]);
    }
    Documents.clear();
    std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](
        const std::pair<TDbDocument, double>& a,
        const std::pair<TDbDocument, double>& b)
    {
        if (std::abs(a.second - b.second) < 0.000001) {
            return a.first.Title < b.first.Title;
        }
        return a.second > b.second;
    });
    for (const auto& [doc, _] : weightedDocs) {
        AddDocument(doc);
    }
}

bool TNewsCluster::operator<(const TNewsCluster& other) const {
    if (FreshestTimestamp == other.FreshestTimestamp) {
        return Id < other.Id;
    }
    return FreshestTimestamp < other.FreshestTimestamp;
}

bool TNewsCluster::Compare(const TNewsCluster& cluster, uint64_t timestamp) {
    return cluster.FreshestTimestamp < timestamp;
}

