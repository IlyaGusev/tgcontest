#include "rank_docs.h"
#include "../util.h"

#include <Eigen/Core>
#include <cassert>

TClustering::TClusters RankClustersDocs(
    const TClustering::TClusters& clusters,
    const TAgencyRating& agencyRating,
    const std::map<std::string, std::unique_ptr<TFastTextEmbedder>>& embedders
) {
    TClustering::TClusters output;
    for (const auto& cluster : clusters) {
        const TFastTextEmbedder& embedder = *embedders.at(cluster.GetLanguage());

        Eigen::MatrixXf points(cluster.GetSize(), embedder.GetEmbeddingSize());
        for (size_t i = 0; i < cluster.GetSize(); i++) {
            const TDocument& doc = cluster.GetDocuments()[i];
            fasttext::Vector embedding = embedder.GetSentenceEmbedding(doc);
            Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
            points.row(i) = eigenVector / eigenVector.norm();
        }
        Eigen::MatrixXf docsCosine = points * points.transpose();

        std::vector<WeightedDoc> weightedDocs;
        uint64_t freshestTimestamp = cluster.GetFreshestTimestamp();
        for (size_t i = 0; i < cluster.GetSize(); ++i) {
            const TDocument& doc = cluster.GetDocuments()[i];
            double docRelevance = docsCosine.row(i).mean();
            double timeMultiplier = Sigmoid((doc.FetchTime - freshestTimestamp) / 3600.0 + 12);
            double agencyScore = agencyRating.ScoreUrl(doc.Url);
            double weight = (agencyScore + docRelevance) * timeMultiplier;
            weightedDocs.emplace_back(doc, weight);
        }
        std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](const WeightedDoc& a, const WeightedDoc& b) {
            return a.Weight > b.Weight;
        });
        TNewsCluster clusterSorted;
        for (const WeightedDoc& elem : weightedDocs) {
            clusterSorted.AddDocument(elem.Doc);
        }
        output.emplace_back(std::move(clusterSorted));
    }
    return output;
}
