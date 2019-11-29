#include "in_cluster_ranging.h"

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
