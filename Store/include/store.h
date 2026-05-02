#ifndef MINI_REDIS_STORE_H
#define MINI_REDIS_STORE_H

// Store header
#pragma once
#include <unordered_map>
#include <string>
#include <fstream>

class Store {
private:
    std::unordered_map<std::string, std::string> cacheDbRedis;
    std::ofstream aof_file;
    std::string aof_filename;
    bool use_fsync;
    std::unordered_map<std::string, std::time_t> cacheExpirtyDb;
    std::mutex redisStoreMutex;

public:
    explicit Store(std::string  aof_file = "data.aof", bool fsync = false);
    void redisAppendToAof(const std::string& command);
    void redisReplayAof(const std::string& filename);
    void redisLoad(const std::string& filename);
    void redisCleanupExpired();
    std::string redisSet(const std::string& key, const std::string& value);
    std::string redisGet(const std::string& key);
    std::string redisDel(const std::string& key);
    std::string redisSetExpire(const std::string &key, const std::string& value, const std::string& ttl_seconds);
    int8_t redisCompactAof();
    bool redisIsExpired(const std::string& key);
    ~Store();
};
#endif