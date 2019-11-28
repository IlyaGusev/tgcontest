#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <fstream>

#include <boost/algorithm/string.hpp> 

std::unordered_map<std::string, double> LoadRatings(std::vector<std::string> ratingFiles); 
