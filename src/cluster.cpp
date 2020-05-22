#include "cluster.h"
#include "document_ranking/weighted_document.h"

#include <boost/range/algorithm/nth_element.hpp>

#include <cassert>
#include <cmath>
#include <set>
#include <vector>

void TNewsCluster::AddDocument(const TDbDocument& document) {
    Documents.push_back(std::cref(document));
}

uint64_t TNewsCluster::GetTimestamp(float percentile) const {
    assert(!Documents.empty());
    std::vector<uint64_t> clusterTimestamps;
    for (const TDbDocument& doc : Documents) {
        clusterTimestamps.push_back(doc.FetchTime);
    }
    size_t index = static_cast<size_t>(std::floor(percentile * (clusterTimestamps.size() - 1)));
    boost::range::nth_element(clusterTimestamps, clusterTimestamps.begin() + index);
    return clusterTimestamps[index];
}

uint64_t TNewsCluster::GetFreshestTimestamp() const {
    return std::max_element(Documents.begin(), Documents.end(), [](const TDbDocument& doc1, const TDbDocument& doc2) {
        return doc1.FetchTime < doc2.FetchTime;
    })->get().FetchTime;
}

tg::ECategory TNewsCluster::GetCategory() const {
    std::vector<size_t> categoryCount(tg::ECategory_ARRAYSIZE);
    for (const TDbDocument& doc : Documents) {
        tg::ECategory docCategory = doc.Category;
        assert(doc.IsNews());
        categoryCount[static_cast<size_t>(docCategory)] += 1;
    }
    auto it = std::max_element(categoryCount.begin(), categoryCount.end());
    return static_cast<tg::ECategory>(std::distance(categoryCount.begin(), it));
}

void TNewsCluster::SortByWeights(const std::vector<double>& weights) {
    std::vector<TWeightedDoc> weightedDocs;
    weightedDocs.reserve(Documents.size());
    for (size_t i = 0; i < Documents.size(); i++) {
        weightedDocs.emplace_back(Documents[i], weights[i]);
    }
    std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](const TWeightedDoc& a, const TWeightedDoc& b) {
        if (std::abs(a.Weight - b.Weight) < 0.000001) {
            return a.Doc.get().Title < b.Doc.get().Title;
        }
        return a.Weight > b.Weight;
    });
    Documents.clear();
    for (const TWeightedDoc& elem : weightedDocs) {
        AddDocument(elem.Doc);
    }
}
