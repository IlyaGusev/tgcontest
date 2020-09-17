#pragma once

#include "clusterer.h"
#include "summarizer.h"

#include <rocksdb/db.h>

class TServerClustering {
public:
    TServerClustering(
        std::unique_ptr<TClusterer> clusterer,
        std::unique_ptr<TSummarizer> summarizer,
        rocksdb::DB* db
    );

    TClusterIndex MakeIndex() const;

private:
    const std::unique_ptr<TClusterer> Clusterer;
    const std::unique_ptr<TSummarizer> Summarizer;
    rocksdb::DB* Db;
};
