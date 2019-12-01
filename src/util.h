#pragma once

#include <string>
#include <vector>

void ReadFileNames(const std::string& directory, std::vector<std::string>& fileNames, int nDocs=-1);
std::string GetHost(const std::string&);
std::string GetCleanFileName(const std::string& fileName);
float Sigmoid(float x);
double Sigmoid(double x);
