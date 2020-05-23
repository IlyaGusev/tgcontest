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

}

void TController::Init(const TContext* context, std::unique_ptr<TAnnotator> annotator) {
    Context = context;
    Annotator = std::move(annotator);
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
    const tinyxml2::XMLError parseCode = html.Parse(req->bodyData(), req->bodyLength());
    if (parseCode != tinyxml2::XML_SUCCESS) {
        MakeSimpleResponse(std::move(callback), drogon::k400BadRequest);
        return;
    }


    const auto getCode = [this, &fname] () -> drogon::HttpStatusCode {
        std::string value;
        const auto mayExist = Context->Db->KeyMayExist(rocksdb::ReadOptions(), fname, &value);
        return mayExist ? drogon::k204NoContent : drogon::k201Created;
    };

    // TODO: return 400 if bad XML
    const boost::optional<TDbDocument> dbDoc = Annotator->AnnotateHtml(html, fname);
    if (!dbDoc) {
        MakeSimpleResponse(std::move(callback), getCode());
        return;
    }

    std::string serializedDoc;
    const bool success = dbDoc->ToProtoString(&serializedDoc);
    if (!success) {
        MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
        return;
    }

    // TODO: possible races while the same fname is provided to multiple queries
    // TODO: use "value_found" flag and check DB instead of only bloom filter
    const drogon::HttpStatusCode code = getCode();
    const rocksdb::Status s = Context->Db->Put(rocksdb::WriteOptions(), fname, serializedDoc);
    if (!s.ok()) {
        MakeSimpleResponse(std::move(callback), drogon::k500InternalServerError);
        return;
    }

    MakeSimpleResponse(std::move(callback), code);
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
        ret["lang"] = doc.language();
        ret["category"] = doc.category();
    }

    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
