#include <cmath>
#include <ctime>
#include <regex>

#include <boost/filesystem.hpp>

#include "util.h"

void ReadFileNames(const std::string& directory, std::vector<std::string>& fileNames, int nDocs) {
    boost::filesystem::path dirPath(directory);
    boost::filesystem::recursive_directory_iterator start(dirPath);
    boost::filesystem::recursive_directory_iterator end;
    for (auto it = start; it != end; it++) {
        if (boost::filesystem::is_directory(it->path())) {
            continue;
        }
        std::string path = it->path().string();
        if (path.substr(path.length() - 5) == ".html") {
            fileNames.push_back(path);
        }
        if (nDocs != -1 && fileNames.size() == static_cast<size_t>(nDocs)) {
            break;
        }
    }
}

std::string GetHost(const std::string& url) {
    // if u know better way to do it - it would be nice if u rewrite it
    std::string output = "";
    // https://stackoverflow.com/questions/2616011/easy-way-to-parse-a-url-in-c-cross-platform
    try {
        static std::regex ex("(http|https)://(?:www\\.)?([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
        std::smatch what;
        if (std::regex_match(url, what, ex) && what.size() >= 3) {
            output = std::string(what[2].first, what[2].second);
        }
    } catch (...) {
        LOG_DEBUG("Can't parse host from url: " << url);
        return output;
    }
    return output;
}

std::string CleanFileName(const std::string& fileName) {
    return fileName.substr(fileName.find_last_of("/") + 1);
}

float Sigmoid(float x) {
    if (x >= 0.0f) {
        float z = exp(-x);
        return 1.0f / (1.0f + z);
    }
    float z = exp(x);
    return z / (1.0f + z);
}

double Sigmoid(double x) {
    if (x >= 0.0) {
        double z = exp(-x);
        return 1.0 / (1.0 + z);
    }
    double z = exp(x);
    return z / (1.0 + z);
}

uint64_t DateToTimestamp(const std::string& date) {
    std::regex ex("(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)T(\\d\\d):(\\d\\d):(\\d\\d)([+-])(\\d\\d):(\\d\\d)");
    std::smatch what;
    if (!std::regex_match(date, what, ex) || what.size() < 10) {
        throw std::runtime_error("wrong date format");
    }
    std::tm t = {};
    t.tm_sec = std::stoi(what[6]);
    t.tm_min = std::stoi(what[5]);
    t.tm_hour = std::stoi(what[4]);
    t.tm_mday = std::stoi(what[3]);
    t.tm_mon = std::stoi(what[2]) - 1;
    t.tm_year = std::stoi(what[1]) - 1900;

    time_t timestamp = timegm(&t);
    uint64_t zone_ts = std::stoi(what[8]) * 60 * 60 + std::stoi(what[9]) * 60;
    if (what[7] == "+") {
        timestamp = timestamp - zone_ts;
    } else if (what[7] == "-") {
        timestamp = timestamp + zone_ts;
    }
    return timestamp > 0 ? timestamp : 0;
}

