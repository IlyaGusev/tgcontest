#include "summarize.h"
#include "util.h"

#include <Eigen/Core>
#include <cassert>

void Summarize(
    TClusters& clusters,
    const TAgencyRating& agencyRating,
    const std::map<std::string, std::unique_ptr<TFastTextEmbedder>>& embedders
) {
    for (auto& cluster : clusters) {
        const TFastTextEmbedder& embedder = *embedders.at(cluster.GetLanguage());

        Eigen::MatrixXf points(cluster.GetSize(), embedder.GetEmbeddingSize());
        for (size_t i = 0; i < cluster.GetSize(); i++) {
            const TDocument& doc = cluster.GetDocuments()[i];
            fasttext::Vector embedding = embedder.GetSentenceEmbedding(doc);
            Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
            points.row(i) = eigenVector / eigenVector.norm();
        }
        Eigen::MatrixXf docsCosine = points * points.transpose();

        std::vector<double> weights;
        weights.reserve(cluster.GetSize());
        uint64_t freshestTimestamp = cluster.GetFreshestTimestamp();
        for (size_t i = 0; i < cluster.GetSize(); ++i) {
            const TDocument& doc = cluster.GetDocuments()[i];
            double docRelevance = docsCosine.row(i).mean();
            double timeMultiplier = Sigmoid((doc.FetchTime - freshestTimestamp) / 3600.0 + 12);
            double agencyScore = agencyRating.ScoreUrl(doc.Url);
            double weight = (agencyScore + docRelevance) * timeMultiplier;
            weights.push_back(weight);
        }
        cluster.SortByWeights(weights);
    }
}
