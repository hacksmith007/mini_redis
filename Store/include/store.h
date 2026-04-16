#ifndef MINI_REDIS_STORE_H
#define MINI_REDIS_STORE_H

// Store header
#pragma once
#include <unordered_map>
#include <string>
#include <fstream>

class Store {
private:
    std::unordered_map<std::string, std::string> db;
    std::ofstream aof_file;
    std::string aof_filename;
    bool use_fsync;

    void append_to_aof(const std::string& command);
    void replay_aof(const std::string& filename);
    void rewrite_aof();

public:
    Store(std::string  aof_file = "data.aof", bool fsync = false);
    ~Store();

    std::string set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    std::string del(const std::string& key);

    void load(const std::string& filename);
    int8_t compact_aof();
};
#endif