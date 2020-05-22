#include "summarize.h"
#include "util.h"

#include <Eigen/Core>
#include <cassert>

void Summarize(
    TClusters& clusters,
    const TAgencyRating& agencyRating
) {
    for (auto& cluster : clusters) {
        const size_t embeddingSize = 50; // TODO: BAD
        Eigen::MatrixXf points(cluster.GetSize(), embeddingSize);
        for (size_t i = 0; i < cluster.GetSize(); i++) {
            const TDbDocument& doc = cluster.GetDocuments()[i];
            auto embedding = doc.Embeddings.at(tg::EK_CLUSTERING);
            Eigen::Map<Eigen::VectorXf, Eigen::Unaligned> eigenVector(embedding.data(), embedding.size());
            points.row(i) = eigenVector / eigenVector.norm();
        }
        Eigen::MatrixXf docsCosine = points * points.transpose();

        std::vector<double> weights;
        weights.reserve(cluster.GetSize());
        uint64_t freshestTimestamp = cluster.GetFreshestTimestamp();
        for (size_t i = 0; i < cluster.GetSize(); ++i) {
            const TDbDocument& doc = cluster.GetDocuments()[i];
            double docRelevance = docsCosine.row(i).mean();
            double timeMultiplier = Sigmoid(static_cast<double>(doc.FetchTime - freshestTimestamp) / 3600.0 + 12.0);
            double agencyScore = agencyRating.ScoreUrl(doc.Url);
            double weight = (agencyScore + docRelevance) * timeMultiplier;
            weights.push_back(weight);
        }
        cluster.SortByWeights(weights);
    }
}
