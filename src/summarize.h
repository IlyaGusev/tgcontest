#pragma once

#include "agency_rating.h"
#include "cluster.h"
#include "embedder.h"

void Summarize(
    TClusters& clusters,
    const TAgencyRating& agencyRating,
    const std::map<std::string, std::unique_ptr<TEmbedder>>& embedders
);
