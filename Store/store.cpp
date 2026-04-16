#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include  <unordered_map>
#include <thread>
#include <mutex>
#include <utility>
#include "store.h"
#include "RedisCommon.h"

std::unordered_map<std::string, std::time_t> expiry;
std::mutex expiry_mutex;

/**
 * ============================================================
 * FUNCTION: is_expired
 * ============================================================
 * Checks whether a given key has expired based on current time.
 * Returns true if expired, false otherwise.
 * ============================================================
 */
bool is_expired(const std::string& key) {
    auto it = expiry.find(key);
    if (it == expiry.end()) return false;
    return std::time(nullptr) > it->second;
}

/**
 * ============================================================
 * FUNCTION: cleanup_expired
 * ============================================================
 * Removes expired keys from the database and expiry map.
 * Must be called periodically (e.g., via scheduler).
 * ============================================================
 */
void cleanup_expired(std::unordered_map<std::string, std::string>& db) {
    std::lock_guard<std::mutex> lock(expiry_mutex);
    if (auto it = expiry.begin(); it != expiry.end()) {
        if (std::time(nullptr) > it->second) {
            db.erase(it->first);
            it = expiry.erase(it);
        } else {
            ++it;
        }
    }
}

/**
 * ============================================================
 * FUNCTION: Store (constructor)
 * ============================================================
 * Initializes the store, opens AOF file, and replays it
 * to rebuild in-memory state.
 * ============================================================
 */
Store::Store(std::string  aof_file_name, const bool fsync)
    : aof_filename(std::move(aof_file_name)), use_fsync(fsync) {

    aof_file.open(aof_filename, std::ios::app);
    if (!aof_file.is_open()) {
        std::cerr << "Warning: Could not open AOF file: " << aof_filename << std::endl;
        REDIS_LOG(INFO, "FAIL AOF_OPEN file=%s", aof_filename.c_str());
    } else {
        REDIS_LOG(INFO, "SUCCESS AOF_OPEN file=%s", aof_filename.c_str());
    }

    replay_aof(aof_filename);
}

/**
 * ============================================================
 * FUNCTION: ~Store (destructor)
 * ============================================================
 * Ensures AOF file is flushed and closed properly.
 * ============================================================
 */
Store::~Store() {
    if (aof_file.is_open()) {
        aof_file.flush();
        aof_file.close();
    }
}

/**
 * ============================================================
 * FUNCTION: append_to_aof
 * ============================================================
 * Appends a command to the AOF file for persistence.
 * Optionally flushes based on fsync configuration.
 * ============================================================
 */
void Store::append_to_aof(const std::string& command) {
    if (!aof_file.is_open()) return;

    aof_file << command;

    if (use_fsync) {
        aof_file.flush(); // ensure durability
    }
}

/**
 * ============================================================
 * FUNCTION: replay_aof
 * ============================================================
 * Replays AOF file to reconstruct in-memory database.
 * Supports SET and DEL commands.
 * ============================================================
 */
void Store::replay_aof(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        REDIS_LOG(INFO, "FAIL AOF_REPLAY_OPEN file=",  filename.c_str());
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            REDIS_LOG(INFO, "FAIL AOF_REPLAY command=<empty> reason=empty_line");
            continue;
        }

        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "SET") {
            std::string key;
            if (!(iss >> key)) {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_key" ,line.c_str());
                continue;
            }

            size_t value_pos = line.find(key) + key.length() + 1;
            if (value_pos < line.length()) {
                std::string value = line.substr(value_pos);
                db[key] = value;
                REDIS_LOG(INFO , "SUCCESS AOF_REPLAY command= %s", line.c_str());
            } else {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_value", line.c_str());
            }
        }
        else if (command == "DEL") {
            std::string key;
            if (!(iss >> key)) {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_key",  line.c_str());
                continue;
            }
            db.erase(key);
            REDIS_LOG(INFO, "SUCCESS AOF_REPLAY command=%s", line.c_str());
        }
        else {
            REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=unknown_command", line.c_str());
        }
    }

    file.close();
    std::cout << "AOF replay complete. Loaded " << db.size() << " keys." << std::endl;
    REDIS_LOG(INFO,  "SUCCESS AOF_REPLAY_COMPLETE file=%s keys=%s", filename.c_str(),  std::to_string(db.size()).c_str());
}

/**
 * ============================================================
 * FUNCTION: set
 * ============================================================
 * Inserts or updates a key-value pair and logs to AOF.
 * ============================================================
 */
std::string Store::set(const std::string& key, const std::string& value) {
    db[key] = value;

    std::string cmd = "SET " + key + " " + value;
    append_to_aof(cmd);

    return "OK";
}

/**
 * ============================================================
 * FUNCTION: get
 * ============================================================
 * Retrieves value for a key if it exists.
 * ============================================================
 */
std::string Store::get(const std::string& key) {
    if (db.count(key)) return db[key];
    return "NULL";
}

/**
 * ============================================================
 * FUNCTION: del
 * ============================================================
 * Deletes a key from the database and logs to AOF.
 * ============================================================
 */
std::string Store::del(const std::string& key) {
    if (db.erase(key)) {
        std::string cmd = "DEL " + key;
        append_to_aof(cmd);
        return "DELETED";
    }
    return "NOT FOUND";
}

/**
 * ============================================================
 * FUNCTION: load
 * ============================================================
 * Loads legacy snapshot format into memory.
 * Used for backward compatibility.
 * ============================================================
 */
void Store::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
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

/**
 * ============================================================
 * FUNCTION: compact_aof
 * ============================================================
 * Rewrites AOF file with current database state to reduce size.
 * Performs atomic replacement of old AOF.
 * ============================================================
 */
int8_t Store::compact_aof() {
    std::string temp_file = aof_filename + ".tmp";
    std::ofstream temp(temp_file, std::ios::trunc);

    if (!temp.is_open()) {
        REDIS_LOG(ERROR, "Could not create temporary AOF file.");
        return -1;
    }

    for (const auto& pair : db) {
        temp << "SET " << pair.first << " " << pair.second << "\n";
    }

    temp.flush();
    temp.close();

    aof_file.close();

    if (std::remove(aof_filename.c_str()) != 0) {
        REDIS_LOG(ERROR, "Could not remove old AOF file.");
        return -1;
    }

    if (std::rename(temp_file.c_str(), aof_filename.c_str()) != 0) {
        REDIS_LOG(ERROR, "Could not rename temporary AOF file.");
        return -1;
    }

    aof_file.open(aof_filename, std::ios::app);
    REDIS_LOG(INFO, "SUCCESS AOF compaction complete. New size=%s keys=%s",  std::to_string(db.size()).c_str() , aof_filename.c_str());

    return 0;
}