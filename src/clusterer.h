#pragma once

#include "cluster.h"
#include "clustering/clustering.h"
#include "config.pb.h"
#include "db_document.h"

#include <vector>
#include <memory>

class TClusterer {
public:
    TClusterer(const std::string& configPath);

    TClusters Cluster(std::vector<TDbDocument>& docs, uint64_t& iterTimestamp) const;

private:
    void ParseConfig(const std::string& fname);

private:
    tg::TClustererConfig Config;
    std::unordered_map<tg::ELanguage, std::unique_ptr<TClustering>> Clusterings;
};
