#include <iostream>

// Parser implementation
#include <sstream>
#include "store.h"

std::string processCommand(const std::string& input, Store& store) {
    std::istringstream iss(input);
    std::string cmd, key, value;

    iss >> cmd >> key;

    if (cmd == "SET") {
        // Read rest of line as value (handles spaces)
        size_t value_pos = input.find(key) + key.length() + 1;
        if (value_pos < input.length()) {
            value = input.substr(value_pos);
            return store.set(key, value);
        }
        return "ERROR: SET requires a value";
    }
    else if (cmd == "GET") {
        return store.get(key);
    }
    else if (cmd == "DEL") {
        return store.del(key);
    }

    return "ERROR: Unknown command";
}