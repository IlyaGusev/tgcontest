#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <boost/algorithm/string.hpp>
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

std::unordered_map<std::string, double> LoadRatings(const std::string& ratingFiles);

double ComputeDocAgencyWeight(
    const TDocument& doc,
    const std::unordered_map<std::string, double>& agencyRating
);

double ComputeDocWeight(
    const TDocument& doc,
    const std::unordered_map<std::string, double>& agencyRating,
    const double docRelevance,
    const uint64_t freshestTimestamp = 0,
    const bool useTimeMultiplier = false
);

std::vector<TNewsCluster> RankClustersDocs(
    const TClustering::TClusters& clusters,
    const std::unordered_map<std::string, double>& agencyRating,
    const TFastTextEmbedder& ruModel,
    const TFastTextEmbedder& enModel
);
