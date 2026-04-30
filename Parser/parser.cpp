// Parser implementation
#include <sstream>
#include "store.h"
#include "commonLibsEnums.h"

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
    std::istringstream iss(input);
    std::string cmd, key, value;

    iss >> cmd;

    if (cmd == "SET") {
        iss >> key;
        size_t value_pos = input.find(key) + key.length() + 1; // extract full value including spaces
        if (value_pos < input.length()) {
            value = input.substr(value_pos);
            return store.set(key, value);
        }
        return "ERROR: SET requires a value";
    }
    else if (cmd == "SETEX") {
        iss >> key;
        std::string ttl_seconds;
        iss >> ttl_seconds;


        size_t value_pos = input.find(key) + key.length() + 1; // extract full value including spaces
        if (value_pos < input.length()) {
            value = input.substr(value_pos);
            return store.setexpire(key, value, ttl_seconds);
        }
        return "ERROR: SET requires a value";
    }
    else if (cmd == "GET") {
        iss >> key;
        return store.get(key);
    }
    else if (cmd == "DEL") {
        return store.del(key);
    }

    return "ERROR: Unknown command";
}

/**
 * ============================================================
 * FUNCTION: redisPollCleanup
 * ============================================================
 * Placeholder for periodic cleanup tasks (e.g., expiry handling).
 * Intended to be invoked by scheduler.
 * ============================================================
 */
RedisStatus redisPollCleanup(Store& store) {
    RedisStatus status = REDIS_STATUS_OK;
    return status;
}