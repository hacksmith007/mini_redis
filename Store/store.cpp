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

// std::unordered_map<std::string, std::time_t> cacheExpirtyDb;
// std::mutex redisStoreMutex;

/**
 * ============================================================
 * FUNCTION: redisIsExpired
 * ============================================================
 * Checks whether a given key has expired based on current time.
 * Returns true if expired, false otherwise.
 * ============================================================
 */
bool Store::redisIsExpired(const std::string& key) {
    auto it = cacheExpirtyDb.find(key);
    if (it == cacheExpirtyDb.end()) return false;
    return std::time(nullptr) > it->second;
}

/**
 * ============================================================
 * FUNCTION: redisCleanupExpired
 * ============================================================
 * Removes expired keys from the database and cacheExpirtyDb map.
 * Must be called periodically (e.g., via scheduler).
 * ============================================================
 */
void Store::redisCleanupExpired() {
    std::lock_guard<std::mutex> lock(redisStoreMutex);
    for (auto it = cacheExpirtyDb.begin(); it != cacheExpirtyDb.end(); ) {
        if (std::time(nullptr) > it->second) {
            REDIS_LOG(DEBUG,"Cleared %s ", it->first.c_str());
            cacheDbRedis.erase(it->first);
            it = cacheExpirtyDb.erase(it);
            if (redisCompactAof()) {
                REDIS_LOG(DEBUG, "Compacted");
            }
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
    REDIS_LOG(DEBUG, "filename %s", aof_filename.c_str());
    aof_file.open(aof_filename, std::ios::app);
    if (!aof_file.is_open()) {
        std::cerr << "Warning: Could not open AOF file: " << aof_filename << std::endl;
        REDIS_LOG(INFO, "FAIL AOF_OPEN file=%s", aof_filename.c_str());
    } else {
        REDIS_LOG(INFO, "SUCCESS AOF_OPEN file=%s", aof_filename.c_str());
    }

      redisReplayAof(aof_filename);
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
 * FUNCTION: redisAppendToAof
 * ============================================================
 * Appends a command to the AOF file for persistence.
 * Optionally flushes based on fsync configuration.
 * ============================================================
 */
void Store::redisAppendToAof(const std::string& command) {
    REDIS_LOG(DEBUG, "command %s", command.c_str());
    std::string clean = command;
    // Remove trailing newline(s)
    while (!clean.empty() && (clean.back() == '\n' || clean.back() == '\r')) {
        clean.pop_back();
    }
    aof_file << clean << std::endl;
    if (use_fsync) {
        REDIS_LOG(INFO, "Flushed");
        aof_file.flush(); // ensure durability
    }
}

/**
 * ============================================================
 * FUNCTION: redisReplayAof
 * ============================================================
 * Replays AOF file to reconstruct in-memory database.
 * Supports SET and DEL commands.
 * ============================================================
 */
void Store::redisReplayAof(const std::string& filename) {
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
                cacheDbRedis[key] = value;
                REDIS_LOG(INFO , "SUCCESS AOF_REPLAY command= %s", line.c_str());
            } else {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_value", line.c_str());
            }
        }
        else if (command == "SETEX") {
            std::string key;
            if (!(iss >> key)) {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_key",line.c_str());
                continue;
            }
            std::string ttl_seconds;
            iss >> ttl_seconds;
            const time_t expire_at = std::time(nullptr) + stoi(ttl_seconds);
            cacheExpirtyDb[key] = expire_at;
            size_t value_pos = line.find(ttl_seconds) + ttl_seconds.length() + 1;
            if (value_pos < line.length()) {
                std::string value = line.substr(value_pos);
                cacheDbRedis[key] = value;

                REDIS_LOG(INFO , "SUCCESS AOF_REPLAY command=%s",line.c_str());
            } else {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_key",line.c_str() );
            }
        }
        else if (command == "DEL") {
            std::string key;
            if (!(iss >> key)) {
                REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=missing_key",  line.c_str());
                continue;
            }
            cacheDbRedis.erase(key);
            REDIS_LOG(INFO, "SUCCESS AOF_REPLAY command=%s", line.c_str());
        }
        else {
            REDIS_LOG(INFO, "FAIL AOF_REPLAY command=%s reason=unknown_command", line.c_str());
        }
    }

    file.close();
    std::cout << "AOF replay complete. Loaded " << cacheDbRedis.size() << " keys." << std::endl;
    REDIS_LOG(INFO,  "SUCCESS AOF_REPLAY_COMPLETE file=%s keys=%s", filename.c_str(),  std::to_string(cacheDbRedis.size()).c_str());
}

/**
 * ============================================================
 * FUNCTION: redisSet
 * ============================================================
 * Inserts or updates a key-value pair and logs to AOF.
 * ============================================================
 */
std::string Store::redisSet(const std::string& key, const std::string& value) {
    cacheDbRedis[key] = value;
    REDIS_LOG(INFO, "storing entry to aof");

    std::string cmd = "SET " + key + " " + value;
    redisAppendToAof(cmd);

    return "OK";
}

std::string Store::redisSetExpire(const std::string &key, const std::string& value, const std::string& ttl_seconds) {
    std::string cmd = "SETEX " + key + " " + ttl_seconds + " " + value;
    REDIS_LOG(INFO, "setex storing to aof %s", cmd.c_str());
    cacheDbRedis[key] = value;

    const time_t expire_at = std::time(nullptr) + std::stoi(ttl_seconds);
    cacheExpirtyDb[key] = expire_at;

    redisAppendToAof(cmd);
    return "OK";
}

/**
 * ============================================================
 * FUNCTION: redisGet
 * ============================================================
 * Retrieves value for a key if it exists.
 * ============================================================
 */
std::string Store::redisGet(const std::string& key) {
    REDIS_LOG(INFO, "getting key=%s", key.c_str());
    if (cacheDbRedis.count(key)) return cacheDbRedis[key];
    return "NULL";
}

/**
 * ============================================================
 * FUNCTION: redisDel
 * ============================================================
 * Deletes a key from the database and logs to AOF.
 * ============================================================
 */
std::string Store::redisDel(const std::string& key) {
    if (cacheDbRedis.erase(key)) {
        std::string cmd = "DEL " + key;
        redisAppendToAof(cmd);
        return "DELETED";
    }
    return "NOT FOUND";
}

/**
 * ============================================================
 * FUNCTION: redisLoad
 * ============================================================
 * Loads legacy snapshot format into memory.
 * Used for backward compatibility.
 * ============================================================
 */
void Store::redisLoad(const std::string& filename) {
    REDIS_LOG(INFO,"LOADING data from file %s", filename.c_str());
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
        REDIS_LOG(DEBUG,"keys %s value %s", key.c_str(), value.c_str());
        cacheDbRedis[key] = value;
    }

    file.close();
}

/**
 * ============================================================
 * FUNCTION: redisCompactAof
 * ============================================================
 * Rewrites AOF file with current database state to reduce size.
 * Performs atomic replacement of old AOF.
 * ============================================================
 */
int8_t Store::redisCompactAof() {
    std::string temp_file = aof_filename + ".tmp";
    std::ofstream temp(temp_file, std::ios::trunc);

    if (!temp.is_open()) {
        REDIS_LOG(ERROR, "Could not create temporary AOF file.");
        return -1;
    }

    for (const auto& pair : cacheDbRedis) {
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
    REDIS_LOG(INFO, "SUCCESS AOF compaction complete. New size=%s keys=%s",  std::to_string(cacheDbRedis.size()).c_str() , aof_filename.c_str());
    return 0;
}