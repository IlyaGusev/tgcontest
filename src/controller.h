#pragma once

#include <drogon/HttpController.h>

class TController : public drogon::HttpController<TController, /* AutoCreation */ false> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::Put,"/{fname}", drogon::Put);
        ADD_METHOD_TO(TController::Delete,"/{fname}", drogon::Delete);
    METHOD_LIST_END

    TController() = default;

    void Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;
    void Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;

private:
};
