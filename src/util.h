#pragma once

#include "enum.pb.h"

#include <fcntl.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <nlohmann_json/json.hpp>

#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#ifdef NDEBUG
#define LOG_DEBUG(x)
#else
#define LOG_DEBUG(x) std::cerr << x << std::endl;
#endif

#define LOG_ERROR(x) std::cerr << x << std::endl;

#define ENSURE(CONDITION, MESSAGE)              \
    do {                                        \
        if (!(CONDITION)) {                     \
            std::ostringstream oss;             \
            oss << MESSAGE;                     \
            throw std::runtime_error(oss.str());\
        }                                       \
    } while (false)

namespace tg {

NLOHMANN_JSON_SERIALIZE_ENUM(tg::ELanguage, {
    {tg::LN_UNDEFINED, nullptr},
    {tg::LN_RU, "ru"},
    {tg::LN_EN, "en"},
    {tg::LN_OTHER, "??"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(tg::ECategory, {
    {tg::NC_UNDEFINED, nullptr},
    {tg::NC_ANY, "any"},
    {tg::NC_SOCIETY, "society"},
    {tg::NC_ECONOMY, "economy"},
    {tg::NC_TECHNOLOGY, "technology"},
    {tg::NC_SPORTS, "sports"},
    {tg::NC_ENTERTAINMENT, "entertainment"},
    {tg::NC_SCIENCE, "science"},
    {tg::NC_OTHER, "other"},
    {tg::NC_NOT_NEWS, "not_news"},
})

}

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
std::string ToString(T e) {
    return nlohmann::json(e);
}

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
T FromString(const std::string& s) {
    return nlohmann::json(s).get<T>();
}

// Read names of all files in directory
void ReadFileNames(const std::string& directory, std::vector<std::string>& fileNames, int nDocs=-1);

// Get host from url
std::string GetHost(const std::string& url);

// Get name of the file without a path to it
std::string CleanFileName(const std::string& fileName);

// Stable sigmoids
float Sigmoid(float x);
double Sigmoid(double x);

// ISO 8601 with timezone date to timestamp
uint64_t DateToTimestamp(const std::string& date);

template <class TConfig>
void ParseConfig(const std::string& fname, TConfig& config) {
    const int fileDesc = open(fname.c_str(), O_RDONLY);
    ENSURE(fileDesc >= 0, "Could not open config file");
    google::protobuf::io::FileInputStream fileInput(fileDesc);
    const bool success = google::protobuf::TextFormat::Parse(&fileInput, &config);
    ENSURE(success, "Invalid prototxt file");
}
