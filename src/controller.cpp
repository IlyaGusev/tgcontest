#include "controller.h"

#include "document.h"
#include "document.pb.h"
#include "util.h"

#include <optional>
#include <tinyxml2/tinyxml2.h>

namespace {
    void MakeSimpleResponse(
        std::function<void(const drogon::HttpResponsePtr&)>&& callback,
        drogon::HttpStatusCode code = drogon::k400BadRequest
    ) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(code);
        callback(resp);
    }

    std::optional<int64_t> ParseTtlHeader(const std::string& value) try {
        if (value == "no-cache") {
            return -1;
        }
        static constexpr size_t PREFIX_LEN = std::char_traits<char>::length("max-age=");
        return static_cast<int64_t>(std::stoi(value.substr(PREFIX_LEN)));
    } catch (const std::exception& e) {
        return std::nullopt;
    }

    std::optional<uint64_t> ParsePeriod(const std::string& value) try {
        return static_cast<uint64_t>(std::stoi(value));
    } catch (const std::exception& e) {
        return std::nullopt;
    }

    std::optional<tg::ELanguage> ParseLang(const std::string& value) {
        const tg::ELanguage lang = FromString<tg::ELanguage>(value);
        return lang != tg::LN_UNDEFINED ? std::make_optional(lang) : std::nullopt;
    }

    std::optional<tg::ECategory> ParseCategory(const std::string& value) {
        const tg::ECategory category = FromString<tg::ECategory>(value);
        return category != tg::NC_UNDEFINED ? std::make_optional(category) : std::nullopt;
    }

    Json::Value ToJson(const TNewsCluster& cluster) {
        Json::Value articles(Json::arrayValue);
        for (const auto& document : cluster.GetDocuments()) {
            articles.append(document.FileName);
        }

        Json::Value json(Json::objectValue);
        json["title"] = cluster.GetTitle();
        json["category"] = ToString(cluster.GetCategory());
        json["articles"] = std::move(articles);
        return json;
    }
}

void TController::Init(
    const THotState<TClusterIndex>* index,
    rocksdb::DB* db,
    std::unique_ptr<TAnnotator> annotator,
    std::unique_ptr<TRanker> ranker
) {
    Index = index;
    Db = db;
    Annotator = std::move(annotator);
    Ranker = std::move(ranker);
    Initialized.store(true, std::memory_order_release);
}

bool TController::IsNotReady(std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    if (Initialized.load(std::memory_order_acquire)) {
        return false;
    }

    MakeSimpleResponse(std::move(callback), drogon::k503ServiceUnavailable);
    return true;
}

std::optional<TDbDocument> TController::ParseDbDocFromReq(
    const drogon::HttpRequestPtr& req,
    const std::string& fname
) const {
    tinyxml2::XMLDocument html;
    const tinyxml2::XMLError parseCode = html.Parse(req->bodyData(), req->bodyLength());
    if (parseCode != tinyxml2::XML_SUCCESS) {
        return std::nullopt;
    }
    return Annotator->AnnotateHtml(html, fname);
}

bool TController::IndexDbDoc(const TDbDocument& dbDoc, const std::string& fname) const {
    ENSURE(dbDoc.IsFullyIndexed(), "Trying to index a document without required fields");
    std::string serializedDoc;
    bool success = dbDoc.ToProtoString(&serializedDoc);
    if (!success) {
        return false;
    }
    // TODO: possible races while the same fname is provided to multiple queries
    // TODO: use "value_found" flag and check DB instead of only bloom filter
    const rocksdb::Status status= Db->Put(rocksdb::WriteOptions(), fname, serializedDoc);
    if (!status.ok()) {
        return false;
    }
    return true;
}

drogon::HttpStatusCode TController::GetCode(
    const std::string& fname,
    drogon::HttpStatusCode createdCode,
    drogon::HttpStatusCode existedCode
) const {
    std::string value;
    const auto mayExist = Db->KeyMayExist(rocksdb::ReadOptions(), fname, &value);
    return mayExist ? existedCode : createdCode;
};


void TController::Put(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& fname
) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    const std::optional<int64_t> ttl = ParseTtlHeader(req->getHeader("Cache-Control"));
    if (!ttl || ttl.value() == -1) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    std::optional<TDbDocument> dbDoc = ParseDbDocFromReq(req, fname);
    if (!dbDoc) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }
    dbDoc->Ttl = ttl.value();

    if (!dbDoc->IsFullyIndexed()) {
        MakeSimpleResponse(std::move(callback), drogon::k204NoContent);
        return;
    }

    const drogon::HttpStatusCode code = GetCode(fname, drogon::k201Created, drogon::k204NoContent);
    bool success = IndexDbDoc(dbDoc.value(), fname);
    if (!success) {
        MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
        return;
    }
    MakeSimpleResponse(std::move(callback), code);
}

void TController::Delete(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& fname
) const {
    UNUSED(req);
    if (IsNotReady(std::move(callback))) {
        return;
    }

    // TODO: possible races while the same fname is provided to multiple queries
    // TODO: use "value_found" flag and check DB instead of only bloom filter
    std::string value;
    const bool mayExist = Db->KeyMayExist(rocksdb::ReadOptions(), fname, &value);
    if (mayExist) {
        const rocksdb::Status s = Db->Delete(rocksdb::WriteOptions(), fname);
        if (!s.ok()) {
            MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
            return;
        }
    }

    MakeSimpleResponse(std::move(callback), mayExist ? drogon::k204NoContent : drogon::k404NotFound);
}

void TController::Threads(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback
) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    const std::optional<uint64_t> period = ParsePeriod(req->getParameter("period"));
    const std::optional<tg::ELanguage> lang = ParseLang(req->getParameter("lang_code"));
    const std::optional<tg::ECategory> category = ParseCategory(req->getParameter("category"));

    if (!period || !lang || !category) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    const std::shared_ptr<TClusterIndex> index = Index->AtomicGet();

    const auto& clusters = index->Clusters.at(lang.value()); // TODO: possible missing key
    const uint64_t fromTimestamp = index->TrueMaxTimestamp > period.value() ? index->TrueMaxTimestamp - period.value() : 0;

    const auto indexIt = std::lower_bound(clusters.cbegin(), clusters.cend(), fromTimestamp, TNewsCluster::Compare);
    const auto weightedClusters = Ranker->Rank(indexIt, clusters.cend(), index->IterTimestamp, period.value());
    const auto& categoryClusters = weightedClusters.at(category.value());

    Json::Value threads(Json::arrayValue);
    int limit = 1000;
    for (const auto& weightedCluster : categoryClusters) {
        if (limit <= 0) {
            break;
        }
        const TNewsCluster& cluster = weightedCluster.Cluster.get();
        threads.append(ToJson(cluster));
        --limit;
    }

    Json::Value json(Json::objectValue);
    json["threads"] = std::move(threads);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    callback(resp);
}

void TController::Get(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& fname
) const {
    UNUSED(req);
    if (IsNotReady(std::move(callback))) {
        return;
    }

    std::string serializedDoc;
    const rocksdb::Status s = Db->Get(rocksdb::ReadOptions(), fname, &serializedDoc);

    Json::Value ret;
    ret["fname"] = fname;
    ret["status"] = s.ok() ? "FOUND" : "NOT FOUND";

    if (s.ok()) {
        tg::TDocumentProto doc;
        const auto suc = doc.ParseFromString(serializedDoc);
        ret["parsed"] = suc;
        ret["title"] = doc.title();
        ret["lang"] = doc.language();
        ret["category"] = doc.category();
        ret["pubtime"] = Json::UInt64(doc.pub_time());
        ret["fetchtime"] = Json::UInt64(doc.fetch_time());
        ret["ttl"] = Json::UInt64(doc.ttl());
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void TController::Post(
    const drogon::HttpRequestPtr& req,
    std::function<void(const drogon::HttpResponsePtr&)>&& callback,
    const std::string& fname
) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    const std::optional<int64_t> ttl = ParseTtlHeader(req->getHeader("Cache-Control"));
    if (!ttl) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    std::optional<TDbDocument> dbDoc = ParseDbDocFromReq(req, fname);
    if (!dbDoc) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    drogon::HttpStatusCode code = GetCode(fname, drogon::k201Created, drogon::k200OK);
    bool isIndexed = dbDoc->IsFullyIndexed() && ttl.value() != -1;
    if (isIndexed) {
        bool success = IndexDbDoc(dbDoc.value(), fname);
        if (!success) {
            MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
            return;
        }
    } else {
        code = drogon::k200OK;
    }

    Json::Value json(Json::objectValue);
    json["lang_code"] = dbDoc->HasSupportedLanguage() ? ToString(dbDoc->Language) : Json::Value::null ;
    json["is_news"] = dbDoc->Category != tg::NC_UNDEFINED ? dbDoc->IsNews() : Json::Value::null;
    json["is_indexed"] = isIndexed;
    Json::Value categories(Json::arrayValue);
    if (dbDoc->IsNews()) {
        categories.append(ToString(dbDoc->Category));
    }
    json["categories"] = categories;

    auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
    resp->setStatusCode(code);
    callback(resp);
}

