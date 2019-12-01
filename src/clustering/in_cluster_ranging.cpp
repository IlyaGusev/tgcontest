#include "../util.h"
#include "in_cluster_ranging.h"
#include "clustering.h"


std::unordered_map<std::string, double> LoadRatings(const std::vector<std::string>& ratingFiles) {
    std::unordered_map<std::string, double> output;
    std::string line;

    for (const auto& ratingPath : ratingFiles) {
        std::ifstream rating(ratingPath);
        if (rating.is_open()) {
            while (std::getline(rating, line)) {
                std::vector<std::string> lineSplitted;
                boost::split(lineSplitted, line, boost::is_any_of("\t"));

                output[lineSplitted[1]] = std::stod(lineSplitted[0]);
            }
        } else {
            std::cerr << "rating file is not available" << std::endl;
        }
    }
    return output;
}

uint64_t GetFreshestTimestamp(const NewsCluster& cluster) {
    uint64_t maxTimestamp = 0;
    for (const auto& doc: cluster) {
        if (doc.get().Timestamp > maxTimestamp) {
            maxTimestamp = doc.get().Timestamp;
        }
    }
    return maxTimestamp;
} 

double ComputeDocWeight(
    const Document& doc,
    const std::unordered_map<std::string, double>& agencyRating,
    const uint64_t freshestTimestamp,
    const bool useTimeMultiplier
) {
    const auto host = GetHost(doc.Url);
    const auto iter = agencyRating.find(host);

    if (iter != agencyRating.end()) {
        // ~ 1 for freshest doc, 0.5 for 12 hour late, ~0 for 24 hour late doc
        const double timeMultiplier = useTimeMultiplier ? Sigmoid((static_cast<double>(doc.Timestamp) - static_cast<double>(freshestTimestamp)) / 3600.0 + 12) : 1.0;
        return iter->second * timeMultiplier;
    } 

    return 0.000015;
}

std::vector<NewsCluster> InClusterRanging(const Clustering::Clusters& clusters, const std::unordered_map<std::string, double>& agencyRating) {
    std::vector<NewsCluster> output;

    for (auto& cluster : clusters) {
        std::vector<WeightedDoc> weightedDocs;
        uint64_t freshestTimestamp = GetFreshestTimestamp(cluster);

        for (const auto& doc : cluster) {
            double weight = ComputeDocWeight(
                doc,
                agencyRating,
                freshestTimestamp,
                /* useTimeMultiplier = */ true
            );
            WeightedDoc weightedDoc(doc, weight);
            weightedDocs.emplace_back(std::move(weightedDoc)); 
        }
        
        std::stable_sort(weightedDocs.begin(), weightedDocs.end(), [](WeightedDoc a, WeightedDoc b) {
            return a.Weight > b.Weight;
        });
        
        NewsCluster clusterSorted;
        std::transform(weightedDocs.cbegin(), weightedDocs.cend(), std::back_inserter(clusterSorted), [](WeightedDoc elem) {
            return elem.Doc;
        });
        output.emplace_back(std::move(clusterSorted));
    }
    return output;
}
