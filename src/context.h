#pragma once

#include <rocksdb/db.h>

#include <memory>

struct TContext {
    const std::unique_ptr<rocksdb::DB> Db;
};
