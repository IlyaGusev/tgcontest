#include "server_clustering.h"

#include "util.h"

TServerClustering::TServerClustering(std::unique_ptr<TClusterer> clusterer, rocksdb::DB* db)
    : Clusterer(std::move(clusterer))
    , Db(db)
{
}

std::vector<TDbDocument> TServerClustering::ReadDocs() const {
    rocksdb::ManagedSnapshot snapshot(Db);

    rocksdb::ReadOptions ropt(/*cksum*/ true, /*cache*/ true);
    ropt.snapshot = snapshot.snapshot();

    std::vector<TDbDocument> docs;

    std::unique_ptr<rocksdb::Iterator> iter(Db->NewIterator(ropt));
    for (iter->SeekToFirst(); iter->Valid(); iter->Next()) {
        const std::string value = iter->value().ToString(); // TODO: use string_view
        if (value.empty()) {
            continue;
        }

        TDbDocument doc;
        const bool succes = TDbDocument::FromProtoString(value, &doc);
        if (!succes) {
            LOG_DEBUG("Bad document in db!")
        }
        docs.push_back(std::move(doc));
    }

    return docs;
}

TClusterIndex TServerClustering::MakeIndex() const {
    std::vector<TDbDocument> docs = ReadDocs();
    std::stable_sort(docs.begin(), docs.end(), [](const TDbDocument& a, const TDbDocument& b) {
        return a.PubTime < b.PubTime;
    });

    TClusterIndex index = Clusterer->Cluster(std::move(docs));

    for (const auto& [lang, clusters] : index.Clusters) {
        LOG_DEBUG("Clustering output: " << lang << " " << clusters.size() << " clusters");
    }

    return index;
};
