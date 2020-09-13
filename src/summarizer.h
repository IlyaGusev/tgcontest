#pragma once

#include "agency_rating.h"
#include "cluster.h"
#include "config.pb.h"

class TSummarizer {
public:
    TSummarizer(const std::string& configPath);

    void Summarize(TClusters& clusters) const;

private:
    tg::TSummarizerConfig Config;
    TAgencyRating AgencyRating;
    TAlexaAgencyRating AlexaAgencyRating;
};
