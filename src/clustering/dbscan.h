#pragma once

#include "clustering.h"

#include <functional>

class Dbscan : public FastTextEmbedder, public Clustering {
public:
    Dbscan(
        fasttext::FastText& model,
        double epsilon,
        size_t minPoints,
        FastTextEmbedder::AggregationMode mode = AM_Avg,
        size_t maxWords = 100,
        const std::string& matrixPath = "",
        const std::string& biasPath = ""
   );

    Clusters Cluster(
        const std::vector<Document>& docs
    ) override;

private:
    const double Epsilon;
    const size_t MinPoints;
};
