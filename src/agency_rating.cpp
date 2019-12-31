#include "agency_rating.h"
#include "util.h"

#include <boost/algorithm/string.hpp>

#include <fstream>

void TAgencyRating::Load(const std::string& filePath, bool setMinAsUnk) {
    std::string line;
    std::ifstream rating(filePath);
    if (!rating.is_open()) {
        LOG_DEBUG("Rating file is not available");
        return;
    }
    while (std::getline(rating, line)) {
        std::vector<std::string> lineSplitted;
        boost::split(lineSplitted, line, boost::is_any_of("\t"));
        Records[lineSplitted[1]] = std::stod(lineSplitted[0]);
    }

    if (setMinAsUnk) {
        UnkRating = std::min_element(Records.begin(), Records.end(),
            [](const std::pair<std::string, double>& item1, const std::pair<std::string, double>& item2) {
                return item1.second < item2.second;
            }
        )->second;
    }
}

double TAgencyRating::ScoreUrl(const std::string& url) const {
    const auto iter = Records.find(GetHost(url));
    return (iter != Records.end()) ? iter->second : UnkRating;
}
