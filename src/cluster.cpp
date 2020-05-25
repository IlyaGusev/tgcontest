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

void TNewsCluster::CalcFeatures(const TAlexaAgencyRating& alexaRating,
    const std::vector<TDbDocument>& docs)
{
    Features.reserve(100);
    for (double shift : {1., 1.3, 1.6}) {
        for (double decay : {1800., 3600., 7200., 86400.}) {
            uint64_t bestTimestamp;
            double importance;
            std::vector<double> docWeights;
            std::map<std::string, double> countryShare;
            std::map<std::string, double> weightedCountryShare;
            CalcImportance(alexaRating,
                docs,
                /*en=*/true,
                /*type=*/0,
                shift,
                decay,
                bestTimestamp,
                importance,
                docWeights,
                countryShare,
                weightedCountryShare);
            Features.push_back(importance);
            if (decay == 86400.) {
                for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
                    Features.push_back(weightedCountryShare[code]);
                }
            }
        }
    }
    for (int type : {1, 2}) {
        for (double decay : {1800., 3600., 7200., 86400.}) {
            uint64_t bestTimestamp;
            double importance;
            std::vector<double> docWeights;
            std::map<std::string, double> countryShare;
            std::map<std::string, double> weightedCountryShare;
            CalcImportance(alexaRating,
                docs,
                /*en=*/true,
                type,
                /*shift=*/0.,
                decay,
                bestTimestamp,
                importance,
                docWeights,
                countryShare,
                weightedCountryShare);
            Features.push_back(importance);
            if (decay == 86400.) {
                for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
                    Features.push_back(weightedCountryShare[code]);
                }
            }
        }
    }
}

void TNewsCluster::CalcImportance(const TAlexaAgencyRating& alexaRating,
    const std::vector<TDbDocument>& docs,
    bool en,
    bool lg,
    double shift,
    double decay,
    uint64_t& bestTimestamp,
    double& importance,
    std::vector<double>& docWeights,
    std::map<std::string, double>& countryShare,
    std::map<std::string, double>& weightedCountryShare)
{
    double count = 0;
    double wCount = 0;
    for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
        countryShare[code] = 0;
        weightedCountryShare[code] = 0;
    }

    DocWeights.reserve(GetSize());
    for (const TDbDocument& doc : Documents) {
		double agencyWeight = alexaRating.ScoreUrl(doc.Host, en, lg, shift);
        docWeights.push_back(agencyWeight);

        const std::string& docHost = doc.Host;;
        for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
            double share = alexaRating.GetCountryShare(docHost, code);
            countryShare[code] += share;
            weightedCountryShare[code] += share * agencyWeight;
        }
        count += 1;
        wCount += agencyWeight;
    }

    for (const std::string& code : {"US", "GB", "IN", "RU", "CA", "AU"}) {
        if (count > 0) {
            countryShare[code] /= count;
        }
        if (wCount > 0) {
            weightedCountryShare[code] /= wCount;
        }
    }

    for (size_t i = 0; i < docs.size(); ++i) {
        const TDbDocument& startDoc = docs[i];
        int32_t startTime = startDoc.FetchTime;
        double rank = 0.;

        std::set<std::string> seenHosts;
        for (size_t j = i; j < docs.size(); ++j) {
            const TDbDocument& doc = docs[j];
            const std::string& docHost = doc.Host;
            if (seenHosts.insert(docHost).second) {
                double agencyWeight = alexaRating.ScoreUrl(docHost, en, lg, shift);
				double docTimestampRemapped = static_cast<double>(startTime - static_cast<int32_t>(doc.FetchTime)) / decay;
                double timeMultiplier = Sigmoid(std::max(docTimestampRemapped, -15.));
                double score = agencyWeight * timeMultiplier;
                rank += score;
            }
        }
        if (rank > importance) {
            importance = rank;
            bestTimestamp = startDoc.FetchTime;
        }
    }
}

void TNewsCluster::CalcImportance(const TAlexaAgencyRating& alexaRating) {
    auto docs = GetDocuments();
    std::stable_sort(docs.begin(), docs.end(), [](const TDbDocument& p1, const TDbDocument& p2) {
        if (p1.FetchTime != p2.FetchTime) {
            return p1.FetchTime < p2.FetchTime;
        }
        return p1.Url < p2.Url;
    });
    CalcFeatures(alexaRating, docs);
    CalcImportance(alexaRating,
        docs,
        /*en=*/true,
        /*lg=*/0,
        /*shift=*/1.,
        /*decay=*/3600,
        BestTimestamp,
        Importance,
        DocWeights,
        CountryShare,
        WeightedCountryShare);
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

