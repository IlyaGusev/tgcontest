#include "controller.h"

#include "document.h"
#include "document.pb.h"

#include <boost/optional.hpp>
#include <tinyxml2/tinyxml2.h>

namespace {

    boost::optional<uint32_t> ParseTtlHeader(const std::string& value) try {
        static constexpr size_t PREFIX_LEN = std::char_traits<char>::length("max-age=");
        return static_cast<uint32_t>(std::stoi(value.substr(PREFIX_LEN)));
    } catch (const std::exception& e) {
        return boost::none;
    }

    void MakeSimpleResponse(std::function<void(const drogon::HttpResponsePtr&)>&& callback, drogon::HttpStatusCode code = drogon::k400BadRequest) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(code);
        callback(resp);
    }

    tg::TDocumentProto ToProto(const TDocument& doc, uint32_t ttl) {
        tg::TDocumentProto proto;
        proto.set_file_name(doc.FileName);
        proto.set_title(doc.Title);
        proto.set_ttl(ttl);
        return proto;
    }
}

void TController::Init(const TContext* context) {
    Context = context;
    Initialized.store(true, std::memory_order_release);
}

bool TController::IsNotReady(std::function<void(const drogon::HttpResponsePtr&)>&& callback) const {
    if (Initialized.load(std::memory_order_acquire)) {
        return false;
    }

    MakeSimpleResponse(std::move(callback), drogon::k503ServiceUnavailable);
    return true;
}

void TController::Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    const boost::optional<uint32_t> ttl = ParseTtlHeader(req->getHeader("Cache-Control"));
    if (!ttl) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    tinyxml2::XMLDocument html;
    const tinyxml2::XMLError code = html.Parse(req->bodyData(), req->bodyLength());
    if (code != tinyxml2::XML_SUCCESS) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }

    TDocument doc(html, fname);
    // put processing here

    std::string serializedDoc;
    ToProto(doc, ttl.value()).SerializeToString(&serializedDoc);

    // TODO: possible races while the same fname is provided to multiple queries
    // TODO: use "value_found" flag and check DB instead of only bloom filter
    std::string value;
    const bool mayExist = Context->Db->KeyMayExist(rocksdb::ReadOptions(), fname, &value);

    const rocksdb::Status s = Context->Db->Put(rocksdb::WriteOptions(), fname, serializedDoc);
    if (!s.ok()) {
        MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
        return;
    }

    Json::Value ret;
    ret["result"] = "put";
    ret["fname"] = fname;
    ret["title"] = doc.Title;
    ret["text"] = doc.Text;
    ret["ttl"] = ttl.value();

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(mayExist ? drogon::k204NoContent : drogon::k201Created);
    callback(resp);
}

void TController::Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    // TODO: possible races while the same fname is provided to multiple queries
    // TODO: use "value_found" flag and check DB instead of only bloom filter
    std::string value;
    const bool mayExist = Context->Db->KeyMayExist(rocksdb::ReadOptions(), fname, &value);
    if (mayExist) {
        const rocksdb::Status s = Context->Db->Delete(rocksdb::WriteOptions(), fname);
        if (!s.ok()) {
            MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
            return;
        }
    }

    MakeSimpleResponse(std::move(callback), mayExist ? drogon::k204NoContent : drogon::k404NotFound);
}

void TController::Get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    std::string serializedDoc;
    const rocksdb::Status s = Context->Db->Get(rocksdb::ReadOptions(), fname, &serializedDoc);

    Json::Value ret;
    ret["fname"] = fname;
    ret["status"] = s.ok() ? "FOUND" : "NOT FOUND";

    if (s.ok()) {
        tg::TDocumentProto doc;
        const auto suc = doc.ParseFromString(serializedDoc);
        ret["parsed"] = suc;
        ret["title"] = doc.title();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
