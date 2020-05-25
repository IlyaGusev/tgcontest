#include "cluster.h"

#include "agency_rating.h"
#include "document_ranking/weighted_document.h"
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

void TNewsCluster::Summarize(const TAgencyRating& agencyRating) {
    assert(GetSize() != 0);
    const size_t embeddingSize = Documents.back().Embeddings.at(tg::EK_FASTTEXT_CLASSIC).size();
    Eigen::MatrixXf points(GetSize(), embeddingSize);
    for (size_t i = 0; i < GetSize(); i++) {
        auto embedding = Documents[i].Embeddings.at(tg::EK_FASTTEXT_CLASSIC);
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
    }
    Eigen::MatrixXf docsCosine = points * points.transpose();

    std::vector<double> weights;
    weights.reserve(GetSize());
    uint64_t freshestTimestamp = GetFreshestTimestamp();
    for (size_t i = 0; i < GetSize(); ++i) {
        const TDbDocument& doc = Documents[i];
        double docRelevance = docsCosine.row(i).mean();
        int64_t timeDiff = static_cast<int64_t>(doc.FetchTime) - static_cast<int64_t>(freshestTimestamp);
        double timeMultiplier = Sigmoid(static_cast<double>(timeDiff) / 3600.0 + 12.0);
        double agencyScore = agencyRating.ScoreUrl(doc.Url);
        double weight = (agencyScore + docRelevance) * timeMultiplier;
        weights.push_back(weight);
    }
    SortByWeights(weights);
}

void TNewsCluster::CalcImportance(const TAlexaAgencyRating& alexaRating) {
    auto docs = GetDocuments();
    std::stable_sort(docs.begin(), docs.end(), [](const TDbDocument& p1, const TDbDocument& p2) {
        if (p1.FetchTime != p2.FetchTime) {
            return p1.FetchTime < p2.FetchTime;
        }
        return p1.Url < p2.Url;
    });

    bool debug = false;

    double count = 0;
    double wCount = 0;
    for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
        CountryShare[code] = 0;
        WeightedCountryShare[code] = 0;
    }

    DocWeights.reserve(GetSize());
    for (const TDbDocument& doc : Documents) {
		double agencyWeight = alexaRating.ScoreUrl(doc.Url, true, true);
        DocWeights.push_back(agencyWeight);

        const std::string& docHost = GetHost(doc.Url);
        for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
            double share = alexaRating.GetCountryShare(docHost, code);
            CountryShare[code] += share;
            WeightedCountryShare[code] += share * agencyWeight;
        }
        count += 1;
        wCount += agencyWeight;

        if (doc.Url == "https://ria.ru/20200508/1571140941.html") {
            debug = true;
        }
    }

    for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
        if (count > 0) {
            CountryShare[code] /= count;
        }
        if (wCount > 0) {
            WeightedCountryShare[code] /= wCount;
        }
    }

    for (size_t i = 0; i < docs.size(); ++i) {
        const TDbDocument& startDoc = docs[i];
        int32_t startTime = startDoc.FetchTime;
        double rank = 0.;

        std::set<std::string> seenHosts;
        for (size_t j = i; j < docs.size(); ++j) {
            const TDbDocument& doc = docs[j];
            const std::string& docHost = GetHost(doc.Url);
            if (seenHosts.insert(docHost).second) {
                double agencyWeight = alexaRating.ScoreUrl(doc.Url, true, true);
				double docTimestampRemapped = static_cast<double>(startTime - static_cast<int32_t>(doc.FetchTime)) / 3600;
                double timeMultiplier = Sigmoid(std::max(docTimestampRemapped, -15.));
                double score = agencyWeight * timeMultiplier;
                rank += score;
            }
        }
        if (rank > Importance) {
            Importance = rank;
            BestTimestamp = startDoc.FetchTime;
        }
    }
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
    std::vector<TWeightedDoc> weightedDocs;
    weightedDocs.reserve(Documents.size());
    for (size_t i = 0; i < Documents.size(); i++) {
        weightedDocs.emplace_back(Documents[i], weights[i]);
    }
    std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](const TWeightedDoc& a, const TWeightedDoc& b) {
        if (std::abs(a.Weight - b.Weight) < 0.000001) {
            return a.Doc.Title < b.Doc.Title;
        }
        return a.Weight > b.Weight;
    });
    Documents.clear();
    for (const TWeightedDoc& elem : weightedDocs) {
        AddDocument(elem.Doc);
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

