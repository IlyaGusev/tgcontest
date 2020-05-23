#pragma once

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
