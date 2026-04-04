#include <iostream>

// Parser implementation
#include <sstream>
#include "store.h"

std::string processCommand(const std::string& input, Store& store) {
    std::istringstream iss(input);
    std::string cmd, key, value;

    iss >> cmd >> key;

    if (cmd == "SET") {
        iss >> value;
        return store.set(key, value);
    }
    else if (cmd == "GET") {
        return store.get(key);
    }
    else if (cmd == "DEL") {
        return store.del(key);
    }

    return "ERROR: Unknown command";
}