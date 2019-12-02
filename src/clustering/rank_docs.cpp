#include <Eigen/Core>
#include <cassert>
#include "../util.h"
#include "rank_docs.h"
#include "clustering.h"


std::unordered_map<std::string, double> LoadRatings(const std::vector<std::string>& ratingFiles) {
    std::unordered_map<std::string, double> output;
    std::string line;

    for (const auto& ratingPath : ratingFiles) {
        std::ifstream rating(ratingPath);
        if (rating.is_open()) {
            while (std::getline(rating, line)) {
                std::vector<std::string> lineSplitted;
                boost::split(lineSplitted, line, boost::is_any_of("\t"));

                output[lineSplitted[1]] = std::stod(lineSplitted[0]);
            }
        } else {
            std::cerr << "rating file is not available" << std::endl;
        }
    }
    return output;
}

uint64_t GetFreshestTimestamp(const NewsCluster& cluster) {
    uint64_t maxTimestamp = 0;
    for (const auto& doc: cluster) {
        if (doc.get().Timestamp > maxTimestamp) {
            maxTimestamp = doc.get().Timestamp;
        }
    }
    return maxTimestamp;
}

double ComputeDocAgencyWeight(
    const Document& doc,
    const std::unordered_map<std::string, double>& agencyRating
) {
    const auto host = GetHost(doc.Url);
    const auto iter = agencyRating.find(host);

    if (iter != agencyRating.end()) {
        return iter->second;
    }

    return 0.000015;
}

double ComputeDocWeight(
    const Document& doc,
    const std::unordered_map<std::string, double>& agencyRating,
    const double docRelevance,
    const uint64_t freshestTimestamp,
    const bool useTimeMultiplier
) {
    // ~ 1 for freshest doc, 0.5 for 12 hour late, ~0 for 24 hour late doc
    const double timeMultiplier = useTimeMultiplier ? Sigmoid((static_cast<double>(doc.Timestamp) - static_cast<double>(freshestTimestamp)) / 3600.0 + 12) : 1.0;

    return (ComputeDocAgencyWeight(doc, agencyRating) + docRelevance) * timeMultiplier;
}

std::vector<NewsCluster> RankClustersDocs(
    const Clustering::Clusters& clusters,
    const std::unordered_map<std::string, double>& agencyRating,
    const FastTextEmbedder& ruModel,
    const FastTextEmbedder& enModel 
) {
    std::vector<NewsCluster> output;

    for (auto& cluster : clusters) {
        std::vector<WeightedDoc> weightedDocs;
        uint64_t freshestTimestamp = GetFreshestTimestamp(cluster);

        Eigen::MatrixXf docsCosine; // NxN matrix with cosine titles

        if (cluster.size() > 0) {
            size_t embSize = cluster[0].get().Language == "ru" ? ruModel.GetEmbeddingSize() : enModel.GetEmbeddingSize();

            Eigen::MatrixXf points(cluster.size(), embSize);
            for (size_t i = 0; i < cluster.size(); i++) {
                const Document& doc = cluster[i];
                if ((doc.Language != "ru") && (doc.Language != "en")) {
                    std::cerr << "Doc " << doc.Url << " has unknown language " << doc.Language << std::endl;
                    assert(1);
                }
                auto& model = doc.Language == "ru" ? ruModel : enModel;
                fasttext::Vector embedding = model.GetSentenceEmbedding(doc);
                Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
                points.row(i) = eigenVector / eigenVector.norm();
            }
            docsCosine = points * points.transpose();
        }

        for (size_t i = 0; i < cluster.size(); ++i) {
            const Document& doc = cluster[i];
            const double docRelevance = docsCosine.row(i).mean();
            double weight = ComputeDocWeight(
                doc,
                agencyRating,
                docRelevance,
                freshestTimestamp,
                /* useTimeMultiplier = */ true
            );
 
            weightedDocs.emplace_back(doc, weight);
        }
        std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](WeightedDoc a, WeightedDoc b) {
            return a.Weight > b.Weight;
        });
        NewsCluster clusterSorted;
        std::transform(weightedDocs.cbegin(), weightedDocs.cend(), std::back_inserter(clusterSorted),
            [] (const WeightedDoc& elem) {
                return elem.Doc;
            }
        );
        output.emplace_back(std::move(clusterSorted));
    }
    return output;
}
