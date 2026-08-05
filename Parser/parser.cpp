// Parser implementation
#include <sstream>
#include "store.h"
#include "commonLibsEnums.h"
#include "include/parser.h"

/**
 * ============================================================
 * FUNCTION: processCommand
 * ============================================================
 * Parses a raw input command string and executes corresponding
 * store operations (SET, GET, DEL).
 *
 * Supports:
 *  - SET key value (value can contain spaces)
 *  - GET key
 *  - DEL key
 *
 * Returns response string based on operation result.
 * ============================================================
 */
std::string processCommand(const std::string& input, Store& store) {
    std::string response;
    processCommand(input, store, response);
    return response;
}

RedisStatus processCommand(const std::string& input, Store& store, std::string &response) {
    std::istringstream iss(input);
    std::string cmd, key, value;

    iss >> cmd;

    if (cmd == "SET") {
        iss >> key;
        size_t value_pos = input.find(key) + key.length() + 1; // extract full value including spaces
        if (value_pos < input.length()) {
            value = input.substr(value_pos);
            RedisStatus s = store.redisSet(key, value);
            if (s == REDIS_STATUS_OK) response = "OK";
            return s;
        }
        return REDIS_STATUS_FAILURE;
    }
    else if (cmd == "SETEX") {
        iss >> key;
        std::string ttl_seconds;
        iss >> ttl_seconds;

        size_t value_pos = input.find(ttl_seconds) + ttl_seconds.length() + 1; // extract full value including spaces
        if (value_pos < input.length()) {
            value = input.substr(value_pos);
            RedisStatus s = store.redisSetExpire(key, value, ttl_seconds);
            if (s == REDIS_STATUS_OK) response = "OK";
            return s;
        }
        return REDIS_STATUS_FAILURE;
    }
    else if (cmd == "GET") {
        iss >> key;
        response = store.redisGet(key);
        return (response == "NULL") ? REDIS_STATUS_NOT_FOUND : REDIS_STATUS_OK;
    }
    else if (cmd == "DEL") {
        iss >> key;
        RedisStatus s = store.redisDel(key);
        if (s == REDIS_STATUS_OK) response = "OK";
        return s;
    }

    return REDIS_STATUS_FAILURE;
}

/**
 * ============================================================
 * FUNCTION: redisPollCleanup
 * ============================================================
 * Placeholder for periodic cleanup tasks (e.g., cacheExpirtyDb handling).
 * Intended to be invoked by scheduler.
 * ============================================================
 */
RedisStatus redisPollCleanup(Store& store) {
    RedisStatus status = REDIS_STATUS_OK;
    return status;
}