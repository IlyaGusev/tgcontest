#include <regex>

#include "in_cluster_ranging.h"
#include "clustering.h"


std::unordered_map<std::string, double> LoadRatings(std::vector<std::string> ratingFiles) {
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

std::string GetHost(std::string url) {
    // if u know better way to do it - it would be nice if u rewrite it
    std::string output = "";
    // https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform
    std::regex ex("(http|https)://(?:www\.)?([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
    std::smatch what;
    if (std::regex_match(url, what, ex) && what.size() >= 3) {
        output = std::string(what[2].first, what[2].second);
    }
    return output;
}

double ComputeDocWeight(const Document& doc, const std::unordered_map<std::string, double>& agencyRating) {
    const auto host = GetHost(doc.Url);
    const auto iter = agencyRating.find(host);
    if (iter != agencyRating.end()) {
        return iter->second;
    }
    return 0.0;
}

std::vector<NewsCluster> InClusterRanging(const Clustering::Clusters& clusters, const std::unordered_map<std::string, double>& agencyRating) {
    std::vector<NewsCluster> output;

    for (auto& cluster : clusters) {
        std::vector<WeightedDoc> weightedDocs;
        for (const auto& doc : cluster) {
            double weight = ComputeDocWeight(doc, agencyRating);
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
