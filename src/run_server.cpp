#include "run_server.h"

#include "annotator.h"
#include "clustering/slink.h"
#include "config.pb.h"
#include "context.h"
#include "controller.h"
#include "db_document.h"
#include "util.h"

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
        ENSURE(fileDesc >= 0, "Could not open config file");

        google::protobuf::io::FileInputStream fileInput(fileDesc);

        tg::TServerConfig config;
        const bool succes = google::protobuf::TextFormat::Parse(&fileInput, &config);
        ENSURE(succes, "Invalid prototxt file");

        return config;
    }

    std::unique_ptr<rocksdb::DB> CreateDatabase(const tg::TServerConfig& config) {
        rocksdb::Options options;
        options.IncreaseParallelism();
        options.OptimizeLevelStyleCompaction();
        options.create_if_missing = !config.db_fail_if_missing();

        rocksdb::DB* db;
        rocksdb::Status s = rocksdb::DB::Open(options, config.db_path(), &db);
        ENSURE(s.ok(), "Failed to create database: " << s.getState());

        return std::unique_ptr<rocksdb::DB>(db);
    }

    TClustersIndex RunClustering(rocksdb::DB* db) {
        const rocksdb::Snapshot* snapshot = db->GetSnapshot();
        rocksdb::ReadOptions ropt(true, true);
        ropt.snapshot = snapshot;
        std::unique_ptr<rocksdb::Iterator> iter(db->NewIterator(ropt));
        std::vector<TDbDocument> docs;
        for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
            TDbDocument doc;
            bool ok = TDbDocument::FromProtoString(iter->value().ToString(), &doc);
            if (!ok) {
                LOG_DEBUG("Bad document in db!")
            }
            docs.push_back(std::move(doc));
        }
        db->ReleaseSnapshot(snapshot);

        std::cerr << "Clustering input: " << docs.size() << " docs" << std::endl;
        TSlinkClustering::TConfig config;
        std::unique_ptr<TClustering> clustering = std::make_unique<TSlinkClustering>(config);
        const TClusters clusters = clustering->Cluster(docs);
        std::cerr << "Clustering output: " << clusters.size() << " clusters" << std::endl;
        TClustersIndex index;
        std::copy(clusters.begin(), clusters.end(), std::inserter(index, index.begin()));
        return index;
    }

}

int RunServer(const std::string& fname) {
    LOG_DEBUG("Loading server config");
    const auto config = ParseConfig(fname);

    LOG_DEBUG("Launching server");
    app()
        .setLogLevel(trantor::Logger::kTrace)
        .addListener("0.0.0.0", config.port())
        .setThreadNum(config.threads());

    auto controllerPtr = std::make_shared<TController>();
    app().registerController(controllerPtr);

    LOG_DEBUG("Creating database");
    std::unique_ptr<rocksdb::DB> db = CreateDatabase(config);

    LOG_DEBUG("Creating annotator");
    std::unique_ptr<TAnnotator> annotator = std::make_unique<TAnnotator>(config.annotator_config_path());

//    RunClustering(context.Db.get());
//    LOG_DEBUG("Initial clustering ok");

    TContext context {
        .Db = std::move(db)
    };

    // call this once clustering is ready
    DrClassMap::getSingleInstance<TController>()->Init(&context, std::move(annotator));

    app().run();

    return 0;
}
