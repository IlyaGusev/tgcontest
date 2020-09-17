#include "summarizer.h"

#include "util.h"

TSummarizer::TSummarizer(const std::string& configPath) {
    ::ParseConfig(configPath, Config);

    // Load agency ratings
    LOG_DEBUG("Loading agency ratings...");
    AgencyRating.Load(Config.hosts_rating());
    LOG_DEBUG("Agency ratings loaded");

    // Load alexa agency ratings
    LOG_DEBUG("Loading alexa agency ratings...");
    AlexaAgencyRating.Load(Config.alexa_rating());
    LOG_DEBUG("Alexa agency ratings loaded");

}

void TSummarizer::Summarize(TClusters& clusters) const {
    for (TNewsCluster& cluster: clusters) {
        assert(cluster.GetSize() > 0);
        cluster.Summarize(AgencyRating);
        cluster.CalcImportance(AlexaAgencyRating);
        cluster.CalcCategory();
    }
}


