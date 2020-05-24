#pragma once

#include "enum.pb.h"

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

#define ENSURE(CONDITION, MESSAGE)              \
    do {                                        \
        if (!(CONDITION)) {                     \
            std::ostringstream oss;             \
            oss << MESSAGE;                     \
            throw std::runtime_error(oss.str());\
        }                                       \
    } while (false)


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

template<typename T, typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
std::string ToString(T e) {
    return nlohmann::json(e);
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
