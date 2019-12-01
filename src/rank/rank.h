#include <unordered_map>
#include <string>
#include <vector>
#include "../clustering/clustering.h"


struct WeightedNewsCluster {
    std::reference_wrapper<const NewsCluster> Cluster;
    std::string Category;
    std::string Title;
    double Weight = 0.0;
    WeightedNewsCluster(const NewsCluster& cluster, const std::string& category, const std::string& title, double weight)
        : Cluster(cluster)
        , Category(category)
        , Title(title)
        , Weight(weight)
    {}

};

std::string ComputeClusterCategory(const NewsCluster& cluster);

double ComputeClusterWeight(
    const NewsCluster& cluster,
    const std::unordered_map<std::string, double>& agencyRating
);

std::unordered_map<std::string, std::vector<WeightedNewsCluster>> Rank(
    const std::vector<NewsCluster>& clusters,
    const std::unordered_map<std::string, double>& agencyRating
);
/*
class Ranker {
private:
    const std::unordered_map<std::string, const RubricThreshold>& Thresholds;
public:
    Ranker(
        std::unordered_map<std::string, RubricThreshold> thresholds
    ) : Tresholds(thresholds)
    {
    }
}

bool IsClusterFromRubric(
    const NewsCluster& cluster,
    const std::string& rubric,
    const double documentThreshold,
    const double clusterThreshold
);

std::unordered_map<std::string, std::vector<NewsCluster>> Rank(
    const std::vector<NewsCluster>& clusters,
    const std::unordered_map<std::string, double>& agencyRating
);

std::vector<NewsCluster> RankRubric(
    const std::vector<NewsCluster>& clusters,
    const std::string& rubric, 
    const std::unordered_map<std::string, double>& agencyRating
);
*/
