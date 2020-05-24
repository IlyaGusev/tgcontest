#include "clusterer.h"
#include "clustering/slink.h"
#include "util.h"

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <fcntl.h>
#include <iostream>


uint64_t GetIterTimestamp(const std::vector<TDbDocument>& documents, double percentile) {
    // In production ts.now() should be here.
    // In this case we have percentile of documents timestamps because of the small percent of wrong dates.
    if (documents.empty()) {
        return 0;
    }
    assert(std::is_sorted(documents.begin(), documents.end(), [](const TDbDocument& d1, const TDbDocument& d2) {
        return d1.FetchTime < d2.FetchTime;
    }));

    size_t index = std::floor(percentile * documents.size());
    return documents[index].FetchTime;
}

TClusterer::TClusterer(const std::string& configPath) {
    ParseConfig(configPath);
    for (const tg::TClusteringConfig& config: Config.clusterings()) {
        Clusterings[config.language()] = std::make_unique<TSlinkClustering>(config);
    }
    // Load agency ratings
    LOG_DEBUG("Loading agency ratings...");
    AgencyRating.Load(Config.hosts_rating());
    LOG_DEBUG("Agency ratings loaded");

    // Load alexa agency ratings
    LOG_DEBUG("Loading alexa agency ratings...");
    AlexaAgencyRating.Load(Config.alexa_rating());
    LOG_DEBUG("Alexa agency ratings loaded");
}

TClusterIndex TClusterer::Cluster(std::vector<TDbDocument>& docs) const {
    std::stable_sort(docs.begin(), docs.end(),
        [](const TDbDocument& d1, const TDbDocument& d2) {
            if (d1.FetchTime == d2.FetchTime) {
                if (d1.FileName.empty() && d2.FileName.empty()) {
                    return d1.Title.length() < d2.Title.length();
                }
                return d1.FileName < d2.FileName;
            }
            return d1.FetchTime < d2.FetchTime;
        }
    );
    TClusterIndex clusterIndex;
    clusterIndex.IterTimestamp = GetIterTimestamp(docs, Config.iter_timestamp_percentile());
    clusterIndex.TrueMaxTimestamp = docs.back().FetchTime;

    std::map<tg::ELanguage, std::vector<TDbDocument>> lang2Docs;
    while (!docs.empty()) {
        const TDbDocument& doc = docs.back();
        if (Clusterings.find(doc.Language) != Clusterings.end()) {
            lang2Docs[doc.Language].push_back(doc);
        }
        docs.pop_back();
    }
    docs.shrink_to_fit();
    docs.clear();

    for (const auto& [language, clustering] : Clusterings) {
        TClusters langClusters = clustering->Cluster(lang2Docs[language]);
        for (TNewsCluster& cluster: langClusters) {
            assert(cluster.GetSize() > 0);
            cluster.Summarize(AgencyRating);
            cluster.CalcImportance(AlexaAgencyRating);
        }
        std::stable_sort(
            langClusters.begin(),
            langClusters.end(),
            [](const TNewsCluster& a, const TNewsCluster& b) {
                return a.GetFreshestTimestamp() < b.GetFreshestTimestamp();
            }
        );
        clusterIndex.Clusters[language] = std::move(langClusters);
    }
    return clusterIndex;
}


void TClusterer::ParseConfig(const std::string& fname) {
    const int fileDesc = open(fname.c_str(), O_RDONLY);
    ENSURE(fileDesc >= 0, "Could not open config file");
    google::protobuf::io::FileInputStream fileInput(fileDesc);
    const bool success = google::protobuf::TextFormat::Parse(&fileInput, &Config);
    ENSURE(success, "Invalid prototxt file");
}
