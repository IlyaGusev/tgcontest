#include "run_server.h"

#include "config.pb.h"
#include "controller.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>

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
    TContext context;

    std::cerr << "Loading server config: " << fname << std::endl;
    const auto config = ParseConfig(fname);

    std::cerr << "Creating database" << std::endl;
    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = !config.dbfailifmissing();

    rocksdb::DB* db;
    rocksdb::Status s = rocksdb::DB::Open(options, config.dbpath(), &db);
    if (!s.ok()) {
        throw std::runtime_error("Failed to create database");
    }
    context.Db = std::unique_ptr<rocksdb::DB>(db);

    std::cerr << "Launching server" << std::endl;
    app()
        .setLogLevel(trantor::Logger::kTrace)
        .addListener("0.0.0.0", config.port())
        .setThreadNum(config.threads());

    auto controllerPtr = std::make_shared<TController>(std::move(context));
    app().registerController(controllerPtr);

    app().run();

    return 0;
}
