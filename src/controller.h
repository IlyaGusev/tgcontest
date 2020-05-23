#pragma once

#include "annotator.h"
#include "cluster.h"
#include "hot_state.h"

#include <drogon/HttpController.h>
#include <rocksdb/db.h>

class TController : public drogon::HttpController<TController, /* AutoCreation */ false> {
public:
    METHOD_LIST_BEGIN
        ADD_METHOD_TO(TController::Threads,"/threads", drogon::Get);
        ADD_METHOD_TO(TController::Put,"/{fname}", drogon::Put);
        ADD_METHOD_TO(TController::Delete,"/{fname}", drogon::Delete);
        ADD_METHOD_TO(TController::Get,"/{fname}", drogon::Get); // debug only
    METHOD_LIST_END

    void Init(
        const THotState<TClustersIndex>* index,
        rocksdb::DB* db,
        std::unique_ptr<TAnnotator> annotator,
        bool skipIrrelevantDocs = false
    );

    void Put(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;
    void Delete(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;
    void Threads(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback) const;
    void Get(const drogon::HttpRequestPtr &req, std::function<void(const drogon::HttpResponsePtr&)> &&callback, const std::string& fname) const;

private:
    bool IsNotReady(std::function<void(const drogon::HttpResponsePtr&)> &&callback) const;

private:
    std::atomic<bool> Initialized {false};

    const THotState<TClustersIndex>* Index;

    rocksdb::DB* Db;
    std::unique_ptr<TAnnotator> Annotator;
    bool SkipIrrelevantDocs = false;
};
