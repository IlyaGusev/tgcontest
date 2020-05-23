#include "server_clustering.h"

#include "util.h"

TServerClustering::TServerClustering(std::unique_ptr<TClustering> clustering, rocksdb::DB* db)
    : Clustering(std::move(clustering))
    , Db(db)
{
}

std::vector<TDbDocument> TServerClustering::ReadDocs() const {
    const rocksdb::Snapshot* snapshot = Db->GetSnapshot();
    rocksdb::ReadOptions ropt(/*cksum*/ true, /*cache*/ true);
    ropt.snapshot = snapshot;

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
    Db->ReleaseSnapshot(snapshot);

    return docs;
}

std::shared_ptr<TClustersIndex> TServerClustering::MakeIndex() const {
    std::vector<TDbDocument> docs = ReadDocs();
    std::stable_sort(docs.begin(), docs.end(), [](const TDbDocument& a, const TDbDocument& b) {
        return a.PubTime < b.PubTime;
    });

    LOG_DEBUG("Clustering input: " << docs.size() << " docs");
    const TClusters clusters = Clustering->Cluster(docs);
    LOG_DEBUG("Clustering output: " << clusters.size() << " clusters");

    std::shared_ptr<TClustersIndex> index = std::make_shared<TClustersIndex>();
    std::move(clusters.begin(), clusters.end(), std::inserter(*index, index->begin()));
    return index;
};
