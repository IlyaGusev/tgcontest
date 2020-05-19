#include "controller.h"

void TController::Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    Json::Value ret;
    ret["result"] = "put";
    ret["fname"] = fname;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}

void TController::Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const {
    Json::Value ret;
    ret["result"] = "delete";
    ret["fname"] = fname;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(ret);
    callback(resp);
}
