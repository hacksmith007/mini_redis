#include "store.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include "parser.h"
#include "RedisCommon.h"

#define PORT 8080

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    Store store;
    store.load("data.db");

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure socket address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Server running on port " << PORT << std::endl;
    REDIS_LOG(INFO, "Server running on port \"" + std::to_string(PORT) + "\"");
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
    
    // Send connection established message to client with newline
    std::string welcome = "Connection established, enter exit to quit\n";
    send(new_socket, welcome.c_str(), welcome.size(), 0);
    REDIS_LOG(INFO, "Client connected");

    char buffer[1024] = {0};

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, 1024);

        if (valread <= 0) {
            std::cout << "Client disconnected" << std::endl;
            REDIS_LOG(INFO, "Client disconnected");
            break;
        }

        // Convert to string and strip trailing whitespace
        std::string command(buffer);
        if (!command.empty() && command.back() == '\n') {
            command.pop_back();
        }
        if (!command.empty() && command.back() == '\r') {
            command.pop_back();
        }

        // Check for exit command
        if (command == "exit") {
            std::cout << "Received exit command, closing connection" << std::endl;
            REDIS_LOG(INFO, "Received exit command, closing connection");
            break;
        }
        if (command == "compact") {
            if (store.compact_aof()) {
                REDIS_LOG(ERROR, "Compact command could not be completed");
            }
            REDIS_LOG(INFO, "Compact command completed");
        }

        std::string response = processCommand(buffer, store);
        response += "\n";
        send(new_socket, response.c_str(), response.size(), 0);
    }

    close(new_socket);
    close(server_fd);

    return 0;
}