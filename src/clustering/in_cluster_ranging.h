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
    std::reference_wrapper<const Document> Doc;
    double Weight = 0.0;
    WeightedDoc(const Document& doc, double weight)
        : Doc(doc)
        , Weight(weight)
    {}
};

std::unordered_map<std::string, double> LoadRatings(const std::vector<std::string>& ratingFiles);

double ComputeDocWeight(
    const Document& doc,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t freshestTimestamp = 0,
    const bool useTimeMultiplier = false
);


std::vector<NewsCluster> InClusterRanging(const Clustering::Clusters& clusters, const std::unordered_map<std::string, double>& agencyRating);
