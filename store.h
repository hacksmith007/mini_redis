#ifndef MINI_REDIS_STORE_H
#define MINI_REDIS_STORE_H

// Store header
#pragma once
#include <unordered_map>
#include <string>

class Store {
private:
    std::unordered_map<std::string, std::string> db;

public:
    std::string set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    std::string del(const std::string& key);
    void save(const std::string& filename);
    void load(const std::string& filename);
};
#endif
