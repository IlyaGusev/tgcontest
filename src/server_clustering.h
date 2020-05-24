#pragma once

#include "clusterer.h"

#include <rocksdb/db.h>

class TServerClustering {
public:
    TServerClustering(std::unique_ptr<TClusterer> clusterer, rocksdb::DB* db);

    TClusterIndex MakeIndex() const;

private:
    const std::unique_ptr<TClusterer> Clusterer;
    rocksdb::DB* Db;
};
