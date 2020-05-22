#pragma once

#include "annotate.h"
#include "context.h"

#include <drogon/HttpController.h>

class TController : public drogon::HttpController<TController, /* AutoCreation */ false> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::Put,"/{fname}", drogon::Put);
        ADD_METHOD_TO(TController::Delete,"/{fname}", drogon::Delete);
        ADD_METHOD_TO(TController::Get,"/{fname}", drogon::Get); // debug only
    METHOD_LIST_END

    void Init(const TContext* context, std::unique_ptr<TAnnotator> annotator);

    void Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;
    void Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;

    void Get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;

private:
    bool IsNotReady(std::function<void(const drogon::HttpResponsePtr&)> &&callback) const;

private:
    std::atomic<bool> Initialized {false};

    std::unique_ptr<TAnnotator> Annotator;
    const TContext* Context;
};
