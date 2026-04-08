#include <iostream>
#include "store.h"

std::string processCommand(const std::string&, Store&);

int main() {
    // AOF file will auto-load on construction
    // fsync=false for performance, true for crash-safety
    Store store("data.aof", false);
    
    std::string input;

    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, input);

        if (input == "exit") break;
        
        if (input == "compact") {
            store.compact_aof();
            std::cout << "AOF compaction triggered." << std::endl;
            continue;
        }

        std::cout << processCommand(input, store) << std::endl;
    }

    return 0;
}