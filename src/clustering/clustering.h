#pragma once

#include "../document.h"

#include "../../thirdparty/fasttext/src/fasttext.h"

#include <functional>
#include <vector>

class Clustering {
public:
    using Clusters = std::vector<std::vector<std::reference_wrapper<const Document>>>;

public:
    Clustering(
        const std::string& embModelPath
    );

    Clusters Cluster(
        const std::vector<Document>& docs,
        double epsilon,
        size_t minPoints
    );

private:
    fasttext::FastText embedder;
};
