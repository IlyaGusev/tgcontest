#include <vector>
#include <Eigen/Core>

class HierarchicalClustering {
public:
    HierarchicalClustering(float distanceThreshold);

    void Cluster(const Eigen::MatrixXf& points);
    const std::vector<size_t>& GetLabels() const;

private:
    const float DistanceThreshold;
    std::vector<size_t> Labels;

    void fillDistanceMatrix(const Eigen::MatrixXf& points, Eigen::MatrixXf& distances);
};
