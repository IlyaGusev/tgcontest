#include "controller.h"

#include "document.h"

#include <tinyxml2/tinyxml2.h>

namespace {

    int ParseTtlHeader(const std::string& value) {
        static constexpr size_t PREFIX_LEN = std::char_traits<char>::length("max-age=");
        return std::stoi(value.substr(PREFIX_LEN));
    }

}

void TController::Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    const std::string& ttlString = req->getHeader("Cache-Control");
    if (ttlString.empty()) {
        throw std::runtime_error("Header Cache-Control is not set");
    }

    tinyxml2::XMLDocument html;
    const tinyxml2::XMLError code = html.Parse(req->bodyData(), req->bodyLength());
    if (code != tinyxml2::XML_SUCCESS) {
        throw std::runtime_error("Invalid documemt");
    }

    TDocument doc(html, fname);

    Json::Value ret;
    ret["result"] = "put";
    ret["title"] = doc.Title;
    ret["text"] = doc.Text;
    ret["ttl"] = ParseTtlHeader(ttlString);
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k201Created); // k204NoContent
    callback(resp);
}

void TController::Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    Json::Value ret;
    ret["result"] = "delete";
    ret["fname"] = fname;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    resp->setStatusCode(drogon::k204NoContent); // k404NotFound
    callback(resp);
}
