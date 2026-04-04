#include "store.h"
#include <iostream>

// Store implementation

std::string Store::set(const std::string& key, const std::string& value) {
    db[key] = value;
    return "OK";
}

std::string Store::get(const std::string& key) {
    if (db.count(key)) return db[key];
    return "NULL";
}

std::string Store::del(const std::string& key) {
    if (db.erase(key)) return "DELETED";
    return "NOT FOUND";
}