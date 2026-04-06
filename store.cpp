#include "store.h"
#include <iostream>
#include <fstream>
#include <sstream>

// Store implementation

std::string Store::set(const std::string& key, const std::string& value) {
    db[key] = value;
    save("data.db");
    return "OK";
}

std::string Store::get(const std::string& key) {
    if (db.count(key)) return db[key];
    return "NULL";
}

std::string Store::del(const std::string& key) {
    if (db.erase(key)) {
        save("data.db");
        return "DELETED";
    }
    return "NOT FOUND";
}

void Store::save(const std::string& filename) {
    std::ofstream file(filename, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return;
    
    for (const auto& pair : db) {
        file << pair.first.length() << ":" << pair.first << "|" 
             << pair.second.length() << ":" << pair.second << "\n";
    }
    file.flush();
    file.close();
}

void Store::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;
    
    std::string line;
    while (std::getline(file, line)) {
        // Format: keylen:key|valuelen:value
        size_t pipe_pos = line.find('|');
        if (pipe_pos == std::string::npos) continue;
        
        std::string key_part = line.substr(0, pipe_pos);
        std::string value_part = line.substr(pipe_pos + 1);
        
        size_t key_colon = key_part.find(':');
        size_t value_colon = value_part.find(':');
        
        if (key_colon == std::string::npos || value_colon == std::string::npos) continue;
        
        std::string key = key_part.substr(key_colon + 1);
        std::string value = value_part.substr(value_colon + 1);
        
        db[key] = value;
    }
    file.close();
}