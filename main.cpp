#include <iostream>
#include "store.h"

std::string processCommand(const std::string&, Store&);

int main() {
    Store store;
    store.load("data.db");
    std::string input;

    while (true) {
        std::cout << ">> ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        std::cout << processCommand(input, store) << std::endl;
    }

    return 0;
}