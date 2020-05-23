#pragma once

#include "clustering/clustering.h"

#include <rocksdb/db.h>

class TServerClustering {
public:
    TServerClustering(std::unique_ptr<TClustering> clustering, rocksdb::DB* db);

    std::shared_ptr<TClustersIndex> MakeIndex() const;

private:
    std::vector<TDbDocument> ReadDocs() const;

private:
    const std::unique_ptr<TClustering> Clustering;
    rocksdb::DB* Db;
};
