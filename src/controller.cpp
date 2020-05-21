#include "controller.h"

#include "document.h"
#include "document.pb.h"

#include <tinyxml2/tinyxml2.h>

namespace {

    uint32_t ParseTtlHeader(const std::string& value) {
        static constexpr size_t PREFIX_LEN = std::char_traits<char>::length("max-age=");
        return static_cast<uint32_t>(std::stoi(value.substr(PREFIX_LEN)));
    }

    tg::TDocumentProto ToProto(const TDocument& doc, uint32_t ttl) {
        tg::TDocumentProto proto;
        proto.set_filename(doc.FileName);
        proto.set_title(doc.Title);
        proto.set_ttl(ttl);
        return proto;
    }
}

void TController::Init(const TContext* context) {
    Context = context;
    Initialized.store(true, std::memory_order_release);
}

bool TController::IsNotReady(std::function<void(const drogon::HttpResponsePtr&)> &&callback) const {
    if (Initialized.load(std::memory_order_acquire)) {
        return false;
    }

    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(drogon::k503ServiceUnavailable);
    callback(resp);
    return true;
}

void TController::Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    const std::string& ttlString = req->getHeader("Cache-Control");
    if (ttlString.empty()) {
        throw std::runtime_error("Header Cache-Control is not set");
    }
    const uint32_t ttl = ParseTtlHeader(ttlString);

    tinyxml2::XMLDocument html;
    const tinyxml2::XMLError code = html.Parse(req->bodyData(), req->bodyLength());
    if (code != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Invalid document");
    }

    TDocument doc(html, fname);
    // put processing here

    std::string serializedDoc;
    ToProto(doc, ttl).SerializeToString(&serializedDoc);

    rocksdb::Status s = Context->Db->Put(rocksdb::WriteOptions(), fname, serializedDoc);
    if (!s.ok()) {
        throw std::runtime_error("Write failed");
    }

    Json::Value ret;
    ret["result"] = "put";
    ret["fname"] = fname;
    ret["title"] = doc.Title;
    ret["text"] = doc.Text;
    ret["ttl"] = ttl;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k201Created); // k204NoContent
    callback(resp);
}

void TController::Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    Json::Value ret;
    ret["result"] = "delete";
    ret["fname"] = fname;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k204NoContent); // k404NotFound
    callback(resp);
}

void TController::Get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    if (IsNotReady(std::move(callback))) {
        return;
    }

    std::string serializedDoc;
    rocksdb::Status s = Context->Db->Get(rocksdb::ReadOptions(), fname, &serializedDoc);

    Json::Value ret;
    ret["result"] = "get";
    ret["fname"] = fname;

    if (s.ok()) {
        tg::TDocumentProto doc;
        const auto suc = doc.ParseFromString(serializedDoc);
        ret["status"] = suc;
        ret["title"] = doc.title();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k204NoContent); // k404NotFound
    callback(resp);
}
