#include "store.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include "parser.h"
#include "RedisCommon.h"
#include "scheduler.h"

#define PORT 8080

/**
 * ============================================================
 * FUNCTION: main
 * ============================================================
 * Entry point of the server.
 *
 * Responsibilities:
 *  - Initialize storage and load persisted data
 *  - Start background scheduler for periodic tasks (e.g. expiry)
 *  - Setup TCP server (socket, bind, listen)
 *  - Accept client connection (blocking call)
 *  - Handle client requests in a loop
 *  - Process commands and send responses
 *  - Cleanup resources on exit
 *
 * Flow:
 *  Store Init → Scheduler Start → Socket Setup → Accept Client
 *      → Read Loop → Process Commands → Send Response → Cleanup
 *
 * Notes:
 *  - `accept()` is blocking (waits for client connection)
 *  - Scheduler runs in a separate thread
 *  - Single-client design (can be extended to multi-client)
 * ============================================================
 */
int main() {
    REDIS_LOG(INFO, "Entrypoint Redis Started");
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize storage and load data from disk
    Store store;
    store.load("data.db");

    // Start Scheduler (background thread)
    // Runs expiry polling every 5 seconds
    Scheduler scheduler;
    std::cout << "Scheduler running" << std::endl;
    scheduler.register_task(expiryPoll, 5000);

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind and listen for incoming connections

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Server running on port " << PORT << std::endl;
    REDIS_LOG(INFO, "Server running on port %s", std::to_string(PORT).c_str());

    // Blocking call: wait for a client to connect
    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    // Send welcome message to client
    std::string welcome = "Connection established, enter exit to quit\n";
    send(new_socket, welcome.c_str(), welcome.size(), 0);
    REDIS_LOG(INFO, "Client connected");

    char buffer[1024] = {0};

    /**
     * ============================================================
     * Client request handling loop
     * ============================================================
     * - Reads input from client
     * - Parses and processes command
     * - Sends response back
     * - Breaks on "exit" or disconnection
     * ============================================================
     */
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, 1024);

        // Handle client disconnect
        if (valread <= 0) {
            std::cout << "Client disconnected" << std::endl;
            REDIS_LOG(INFO, "Client disconnected");
            break;
        }


        //Clean input (remove newline / carriage return)
        std::string command(buffer);
        if (!command.empty() && command.back() == '\n') {
            command.pop_back();
        }
        if (!command.empty() && command.back() == '\r') {
            command.pop_back();
        }


        //Exit command handling
        if (command == "exit") {
            std::cout << "Received exit command, closing connection" << std::endl;
            REDIS_LOG(INFO, "Received exit command, closing connection");
            break;
        }

        // Compact AOF (Append Only File)
        if (command == "compact") {
            if (store.compact_aof()) {
                REDIS_LOG(ERROR, "Compact command could not be completed");
            }
            REDIS_LOG(INFO, "Compact command completed");
        }

        //Process command and generate response
        std::string response = processCommand(buffer, store);
        response += "\n";

        //Send response back to client
        send(new_socket, response.c_str(), response.size(), 0);
    }

    // Cleanup resources
    close(new_socket);
    close(server_fd);

    return 0;
}