#ifndef MINI_REDIS_STORE_H
#define MINI_REDIS_STORE_H

#pragma once
// Store header
#include "commonLibsEnums.h"
#include <filesystem>
#include <unordered_map>
#include <string>
#include <fstream>
#include <mutex>
#include <ctime>

class Store {
private:
    std::unordered_map<std::string, std::string> cacheDbRedis;
    std::ofstream aof_file;
    std::string aof_filename;
    bool use_fsync;
    std::unordered_map<std::string, std::time_t> cacheExpirtyDb;
    std::mutex redisStoreMutex;
    size_t max_aof_size = 64 * 1024 * 1024; // 64 MB
    size_t last_compaction_size = 0;
    size_t current_aof_size = 0;

public:
    static Store& getInstance(const std::string& aof_file = "data.aof", bool fsync = false);
    Store(const Store&) = delete;
    Store& operator=(const Store&) = delete;
    Store(Store&&) = delete;
    Store& operator=(Store&&) = delete;
    ~Store();

private:
    explicit Store(std::string  aof_file = "data.aof", bool fsync = false);

public:
    // API for Redis-like operations //
    RedisStatus redisSet(const std::string& key, const std::string& value);
    std::string redisGet(const std::string& key);
    RedisStatus redisDel(const std::string& key);
    RedisStatus redisSetExpire(const std::string &key, const std::string& value, const std::string& ttl_seconds);
    // Helper APIs //
    void redisAppendToAof(const std::string& command);
    void redisReplayAof(const std::string& filename);
    void redisLoad(const std::string& filename);
    void redisCleanupExpired();
    int8_t redisCompactAof();
    uint64_t getCacheSize() const { return cacheDbRedis.size(); }
    size_t getAofFileSize() const;
    bool redisIsExpired(const std::string& key);
    void saveSnapshot(const std::string& filename);
    int saveSnapshotWithFork(const std::string& filename);
};
#endif