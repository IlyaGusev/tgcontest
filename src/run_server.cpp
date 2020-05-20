#include "run_server.h"

#include "config.pb.h"
#include "controller.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include <fcntl.h>
#include <iostream>

using namespace drogon;

namespace {

    tg::TServerConfig ParseConfig(const std::string& fname) {
        const int fileDesc = open(fname.c_str(), O_RDONLY);
        if (fileDesc < 0) {
            throw std::runtime_error("Could not open config file");
        }

        google::protobuf::io::FileInputStream fileInput(fileDesc);

        tg::TServerConfig config;
        if (!google::protobuf::TextFormat::Parse(&fileInput, &config)) {
            throw std::runtime_error("Invalid prototxt file");
        }

        return config;
    }

}

int RunServer(const std::string& fname) {
    std::cerr << "Loading server config: " << fname << std::endl;
    const auto config = ParseConfig(fname);

    app()
        .setLogLevel(trantor::Logger::kTrace)
        .addListener("0.0.0.0", config.port())
        .setThreadNum(config.threads());

    auto controllerPtr = std::make_shared<TController>();
    app().registerController(controllerPtr);

    app().run();

    return 0;
}
