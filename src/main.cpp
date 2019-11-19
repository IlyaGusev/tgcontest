#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>

namespace po = boost::program_options;

int main(int argc, char** argv) {
    po::options_description desc("options");
    desc.add_options()
        ("mode", po::value<std::string>()->required(), "mode")
        ;

    po::positional_options_description p;
    p.add("mode", 1);

    po::command_line_parser parser{argc, argv};
    parser.options(desc).positional(p).allow_unregistered();
    po::parsed_options parsed_options = parser.run();
    po::variables_map vm;
    po::store(parsed_options, vm);
    po::notify(vm);
    if (vm.count("mode")) {
        auto mode = vm["mode"].as<std::string>();
        std::cout << "Mode: " << mode << std::endl;
    }
}
