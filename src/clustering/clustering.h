#pragma once

#include "../document.h"

#include <fasttext.h>
#include <Eigen/Core>

typedef std::vector<std::reference_wrapper<const Document>> NewsCluster;

class Clustering {
public:
    using Clusters = std::vector<NewsCluster>;

public:
    virtual Clusters Cluster(const std::vector<Document>& docs) = 0;
    virtual ~Clustering() = default;
};


class FastTextEmbedder {
public:
    enum AggregationMode {
        AM_Avg = 0,
        AM_Max = 1,
        AM_Min = 2,
        AM_Matrix = 3
    };

    FastTextEmbedder(
        fasttext::FastText& model,
        AggregationMode mode = AM_Avg,
        size_t maxWords = 100,
        const std::string& matrixPath = "",
        const std::string& biasPath = "");
    virtual ~FastTextEmbedder() = default;

    size_t GetEmbeddingSize() const;
    fasttext::Vector GetSentenceEmbedding(const Document& str);

private:
    fasttext::FastText& Model;
    AggregationMode Mode;
    size_t MaxWords;
    Eigen::MatrixXf Matrix;
    Eigen::VectorXf Bias;
};
