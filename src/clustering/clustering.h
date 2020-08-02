#pragma once

#include "../cluster.h"
#include "../db_document.h"

class TClustering {
public:
    TClustering() = default;
    virtual ~TClustering() = default;
    virtual TClusters Cluster(
        const std::vector<TDbDocument>& docs
    ) = 0;
};
