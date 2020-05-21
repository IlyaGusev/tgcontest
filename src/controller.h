#pragma once

#include "context.h"

#include <drogon/HttpController.h>

class TController : public drogon::HttpController<TController, /* AutoCreation */ false> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::Put,"/{fname}", drogon::Put);
        ADD_METHOD_TO(TController::Delete,"/{fname}", drogon::Delete);
        ADD_METHOD_TO(TController::Get,"/{fname}", drogon::Get); // debug only
    METHOD_LIST_END

    explicit TController(const TContext* context);

    void Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;
    void Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;

    void Get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;

private:
    const TContext* Context;
};
