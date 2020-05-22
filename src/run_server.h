#pragma once

#include <boost/program_options.hpp>

#include <string>

int RunServer(const std::string& fname, const boost::program_options::variables_map& vm);
