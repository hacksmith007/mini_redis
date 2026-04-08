#include "store.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <cstdio>
#include <ctime>

namespace {
void log_replay_event(const std::string& message) {
    std::ofstream log_file("redis.log", std::ios::app);
    if (!log_file.is_open()) {
        return;
    }

    std::time_t now = std::time(nullptr);
    char timestamp[32];
    if (std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now))) {
        log_file << "[" << timestamp << "] ";
    }
    log_file << message << "\n";
}
}

// AOF format: SET key value\n or DEL key\n

Store::Store(const std::string& aof_file_name, bool fsync) 
    : aof_filename(aof_file_name), use_fsync(fsync) {
    // Open AOF file in append mode
    aof_file.open(aof_filename, std::ios::app);
    if (!aof_file.is_open()) {
        std::cerr << "Warning: Could not open AOF file: " << aof_filename << std::endl;
        log_replay_event("FAIL AOF_OPEN file=" + aof_filename);
    } else {
        log_replay_event("SUCCESS AOF_OPEN file=" + aof_filename);
    }
    
    // Replay existing AOF to rebuild state
    replay_aof(aof_filename);
}

Store::~Store() {
    if (aof_file.is_open()) {
        aof_file.flush();
        aof_file.close();
    }
}

void Store::append_to_aof(const std::string& command) {
    if (!aof_file.is_open()) return;
    
    aof_file << command ;
    
    if (use_fsync) {
        aof_file.flush();
        // Note: Direct fsync requires platform-specific file descriptor access
        // For simplicity, we rely on flush() for now
    }
}

void Store::replay_aof(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        log_replay_event("FAIL AOF_REPLAY_OPEN file=" + filename);
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            log_replay_event("FAIL AOF_REPLAY command=<empty> reason=empty_line");
            continue;
        }
        
        std::istringstream iss(line);
        std::string command;
        iss >> command;
        
        if (command == "SET") {
            std::string key, value;
            if (!(iss >> key)) {
                log_replay_event("FAIL AOF_REPLAY command=\"" + line + "\" reason=missing_key");
                continue;
            }
            // Rest of line is the value (handles spaces in values)
            size_t value_pos = line.find(key) + key.length() + 1;
            if (value_pos < line.length()) {
                value = line.substr(value_pos);
                db[key] = value;
                log_replay_event("SUCCESS AOF_REPLAY command=\"" + line + "\"");
            } else {
                log_replay_event("FAIL AOF_REPLAY command=\"" + line + "\" reason=missing_value");
            }
        } 
        else if (command == "DEL") {
            std::string key;
            if (!(iss >> key)) {
                log_replay_event("FAIL AOF_REPLAY command=\"" + line + "\" reason=missing_key");
                continue;
            }
            db.erase(key);
            log_replay_event("SUCCESS AOF_REPLAY command=\"" + line + "\"");
        }
        else {
            log_replay_event("FAIL AOF_REPLAY command=\"" + line + "\" reason=unknown_command");
        }
    }
    
    file.close();
    std::cout << "AOF replay complete. Loaded " << db.size() << " keys." << std::endl;
    log_replay_event("SUCCESS AOF_REPLAY_COMPLETE file=" + filename + " keys=" + std::to_string(db.size()));
}

std::string Store::set(const std::string& key, const std::string& value) {
    db[key] = value;
    
    // Append to AOF
    std::string cmd = "SET " + key + " " + value;
    append_to_aof(cmd);
    
    return "OK";
}

std::string Store::get(const std::string& key) {
    if (db.count(key)) return db[key];
    return "NULL";
}

std::string Store::del(const std::string& key) {
    if (db.erase(key)) {
        // Append to AOF
        std::string cmd = "DEL " + key;
        append_to_aof(cmd);
        return "DELETED";
    }
    return "NOT FOUND";
}

void Store::load(const std::string& filename) {
    // For backward compatibility with RDB format
    // This can load an old-style snapshot if needed
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

void Store::compact_aof() {
    // Create a new AOF with current state (rewrite)
    std::string temp_file = aof_filename + ".tmp";
    std::ofstream temp(temp_file, std::ios::trunc);
    
    if (!temp.is_open()) {
        std::cerr << "Error: Could not create temporary AOF file." << std::endl;
        return;
    }
    
    // Write current state as SET commands
    for (const auto& pair : db) {
        temp << "SET " << pair.first << " " << pair.second << "\n";
    }
    
    temp.flush();
    temp.close();
    
    // Atomically replace old AOF with new one
    aof_file.close();
    
    if (std::remove(aof_filename.c_str()) != 0) {
        std::cerr << "Error: Could not remove old AOF file." << std::endl;
        return;
    }
    
    if (std::rename(temp_file.c_str(), aof_filename.c_str()) != 0) {
        std::cerr << "Error: Could not rename temporary AOF file." << std::endl;
        return;
    }
    
    // Reopen AOF for appending
    aof_file.open(aof_filename, std::ios::app);
    std::cout << "AOF compaction complete. New size: " << db.size() << " keys." << std::endl;
}
