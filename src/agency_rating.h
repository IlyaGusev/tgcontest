#pragma once

#include <string>
#include <unordered_map>

class TAgencyRating {
public:
    TAgencyRating() = default;
    TAgencyRating(const std::string& fileName, bool setMinAsUnk = false) {
        Load(fileName, setMinAsUnk);
    }

    void Load(const std::string& fileName, bool setMinAsUnk = false);
    double ScoreUrl(const std::string& url) const;

private:
     std::unordered_map<std::string, double> Records;
     double UnkRating = 0.000015;
};
