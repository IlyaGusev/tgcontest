#pragma once

#include "../agency_rating.h"
#include "clustering.h"
#include "../document.h"

struct WeightedDoc {
    std::reference_wrapper<const TDocument> Doc;
    double Weight = 0.0;
    WeightedDoc(const TDocument& doc, double weight)
        : Doc(doc)
        , Weight(weight)
    {}
};

TClustering::TClusters RankClustersDocs(
    const TClustering::TClusters& clusters,
    const TAgencyRating& agencyRating,
    const std::map<std::string, std::unique_ptr<TFastTextEmbedder>>& embedders
);
