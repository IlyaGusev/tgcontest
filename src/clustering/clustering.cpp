#include "clustering.h"

#include <mlpack/core.hpp>
#include <mlpack/methods/dbscan/dbscan.hpp>

#include <stdint.h>

Clustering::Clustering(
    const std::string& embModelPath
) {
    embedder.loadModel(embModelPath);
}

Clustering::Clusters Clustering::Cluster(
    const std::vector<Document>& docs,
    double epsilon,
    size_t minPoints
) {
    const size_t docSize = docs.size();
    const size_t embSize = embedder.getDimension();

    arma::mat data(embSize, docSize);

    for (size_t i = 0; i < docSize; ++i) {
        std::istringstream ss(docs[i].Title);

        fasttext::Vector embedding(embSize);
        embedder.getSentenceVector(ss, embedding);

        arma::fcolvec fvec(embedding.data(), embSize, /*copy_aux_mem*/ false, /*strict*/ true);
        data.col(i) = arma::conv_to<arma::colvec>::from(fvec);
    }

    mlpack::dbscan::DBSCAN<> clustering(epsilon, minPoints);

    arma::Row<size_t> assignments;
    const size_t clustersSize = clustering.Cluster(data, assignments);

    Clustering::Clusters clusters;
    clusters.resize(clustersSize);

    for (size_t i = 0; i < docSize; ++i) {
        const size_t clusterId = assignments[i];
        if (clusterId == SIZE_MAX) { // outlier
            clusters.push_back({std::cref(docs[i])});
            continue;
        }
        clusters[clusterId].push_back(std::cref(docs[i]));
    }

    return clusters;
}
