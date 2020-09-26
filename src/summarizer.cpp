#include "summarizer.h"

#include "util.h"

static constexpr std::array<const char*, 6> TSummarizer::RegionCodes {{
    "US", "GB", "IN", "RU", "CA", "AU"
}};

static constexpr std::array<double, 4> TSummarizer::Decays {{
    1800., 3600., 7200., 86400.
}};

static constexpr std::array<double, 3> TSummarizer::Shifts {{
    1., 1.3, 1.6
}};


TSummarizer::TSummarizer(const std::string& configPath) {
    ::ParseConfig(configPath, Config);

    // Load agency ratings
    LOG_DEBUG("Loading agency ratings...");
    AgencyRating.Load(Config.hosts_rating());
    LOG_DEBUG("Agency ratings loaded");

    // Load alexa agency ratings
    LOG_DEBUG("Loading alexa agency ratings...");
    AlexaAgencyRating.Load(Config.alexa_rating());
    LOG_DEBUG("Alexa agency ratings loaded");

}

TSummarizedClusters TSummarizer::Summarize(const TClusters& clusters) const {
    for (TNewsCluster& cluster: clusters) {
        assert(cluster.GetSize() > 0);
        RankDocuments(cluster, AgencyRating);
        CalcImportance(cluster, AlexaAgencyRating);
        CalcCategory(cluster);
    }
}

TSummarizedCluster TSummarizer::RankDocuments(const TNewsCluster& cluster, const TAgencyRating& agencyRating) {
    const size_t embeddingSize = cluster.GetDocuments().back().Embeddings.at(tg::EK_FASTTEXT_CLASSIC).size();
    Eigen::MatrixXf points(cluster.GetSize(), embeddingSize);
    for (size_t i = 0; i < cluster.GetSize(); i++) {
        auto embedding = cluster.GetDocuments()[i].Embeddings.at(tg::EK_FASTTEXT_CLASSIC);
        Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
        points.row(i) = eigenVector / eigenVector.norm();
    }
    Eigen::MatrixXf docsCosine = points * points.transpose();

    std::vector<std::pair<double, size_t>> weights;
    weights.reserve(cluster.GetSize());
    TSummarizedCluster summarizedCluster(cluster);
    uint64_t freshestTimestamp = cluster.GetFreshestTimestamp();
    for (size_t i = 0; i < cluster.GetSize(); ++i) {
        const TDbDocument& doc = Documents[i];
        double docRelevance = docsCosine.row(i).mean();
        int64_t timeDiff = static_cast<int64_t>(doc.FetchTime) - static_cast<int64_t>(freshestTimestamp);
        double timeMultiplier = Sigmoid(static_cast<double>(timeDiff) / 3600.0 + 12.0);
        double agencyScore = agencyRating.ScoreUrl(doc.Url);
        double weight = (agencyScore + docRelevance) * timeMultiplier;
        if (doc.Nasty) {
            weight *= 0.5;
        }
        weights.emplace_back(weight, i);
    }
    SortByWeights(weights);
    return summarizedCluster
}

void TSummarizer::CalcImportance(TNewsCluster& cluster, const TAlexaAgencyRating& alexaRating) {
    auto docs = cluster.GetDocuments();
    std::stable_sort(docs.begin(), docs.end(), [](const TDbDocument& p1, const TDbDocument& p2) {
        if (p1.FetchTime != p2.FetchTime) {
            return p1.FetchTime < p2.FetchTime;
        }
        return p1.Url < p2.Url;
    });
    CalcFeatures(docs, alexaRating);
    /*TSliceFeatures slice = CalcImportance(alexaRating, docs, tg::LN_EN, RT_LOG, 1., 3600);
    BestTimestamp = slice.BestTimestamp;
    Importance = slice.Importance;
    DocWeights = slice.DocWeights;
    CountryShare = slice.CountryShare;
    WeightedCountryShare = slice.WeightedCountryShare; */
}

void TSummarizer::CalcFeatures(
    const std::vector<TDbDocument>& docs,
    const TAlexaAgencyRating& alexaRating
) {
    TVector<double> features;
    Features.reserve(Shifts.size()*Decays.size()*RegionCodes.size() + 2*Decays.size()*RegionCodes.size());
    for (double shift : Shifts) {
        for (double decay : Decays) {
            auto slice = CalcImportance(docs, alexaRating, tg::LN_EN, RT_LOG, shift, decay);
            Features.push_back(slice.Importance);
            if (decay != 86400.) {
                continue;
            }
            for (const std::string& code : RegionCodes) {
                Features.push_back(slice.WeightedCountryShare[code]);
            }
        }
    }
    for (ERatingType type : {RT_RAW, RT_ONE}) {
        for (double decay : decays) {
            auto slice = CalcImportance(docs, alexaRating, tg::LN_EN, type, 0.0, decay);
            Features.push_back(slice.Importance);
            if (decay != 86400.) {
                continue;
            }
            for (const std::string& code : codes) {
                Features.push_back(slice.WeightedCountryShare[code]);
            }
        }
    }
}

TSliceFeatures TSummarizer::CalcImportance(
    const std::vector<TDbDocument>& docs,
    const TAlexaAgencyRating& alexaRating,
    tg::ELanguage language,
    ERatingType type,
    double shift,
    double decay)
{
    TSliceFeatures slice;
    for (const std::string& code : codes) {
        slice.CountryShare[code] = 0.0;
        slice.WeightedCountryShare[code] = 0.0;
    }

    size_t docCount = docs.size();
    slice.DocWeights.reserve(docCount);

    double sumWeight = 0.0;
    for (const TDbDocument& doc : docs) {
        const std::string& host = doc.Host;
        double agencyWeight = alexaRating.ScoreUrl(host, language, type, shift);
        slice.DocWeights.push_back(agencyWeight);
        for (const std::string& code : codes) {
            double share = alexaRating.GetCountryShare(host, code);
            slice.CountryShare[code] += (share / docCount);
            slice.WeightedCountryShare[code] += share * agencyWeight;
        }
        sumWeight += agencyWeight;
    }

    if (sumWeight > 0.0) {
        for (const std::string& code : codes) {
            slice.WeightedCountryShare[code] /= sumWeight;
        }
    }

    for (size_t i = 0; i < docs.size(); ++i) {
        const TDbDocument& startDoc = docs[i];
        int32_t startTime = startDoc.FetchTime;
        double rank = 0.0;

        std::set<std::string> seenHosts;
        for (size_t j = i; j < docs.size(); ++j) {
            const TDbDocument& doc = docs[j];
            const std::string& host = doc.Host;
            if (!seenHosts.insert(host).second) {
                continue;
            }
            double agencyWeight = alexaRating.ScoreUrl(host, language, type, shift);
            int32_t diff = startTime - static_cast<int32_t>(doc.FetchTime);
            double docTimestampRemapped = static_cast<double>(diff) / decay;
            double timeMultiplier = Sigmoid(std::max(docTimestampRemapped, -15.));
            double score = agencyWeight * timeMultiplier;
            rank += score;
        }
        if (rank > slice.Importance) {
            slice.Importance = rank;
            slice.BestTimestamp = startDoc.FetchTime;
        }
    }
    return slice;
}

