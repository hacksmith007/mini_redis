#include "store.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include "parser.h"
#include "RedisCommon.h"
#include "scheduler.h"
#include "utility.h"
#define PORT 8080
int port;

/**
 * ============================================================
 * FUNCTION: main
 * ============================================================
 * Entry point of the server.
 *
 * Responsibilities:
 *  - Initialize storage and load persisted data
 *  - Start background scheduler for periodic tasks (e.g. cacheExpirtyDb)
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
    port = getAttributeValue("redis.conf", "port").empty() ? PORT : std::stoi(getAttributeValue("redis.conf", "port"));
    log_level_global = getAttributeValue("redis.conf", "log_level");
    std::cout << "Log Level: " << log_level_global << std::endl;
    REDIS_LOG(INFO, "Entrypoint Redis Started");
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize storage and load persisted data from disk
    Store& store = Store::getInstance("data.aof", getAttributeValue("redis.conf", "fsync").empty() ? false : (getAttributeValue("redis.conf", "fsync") == "true"));
    store.redisLoad("data.cacheDbRedis");

    // Start Scheduler (background thread)
    // Runs cacheExpirtyDb polling every 5 seconds
    Scheduler scheduler;
    REDIS_LOG(INFO, "Scheduler started");
    scheduler.register_task([&store]() { expiryPoll(store); }, 5000);
    scheduler.register_task([&store]() { store.saveSnapshotWithFork("database.rdb"); }, 5000);

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    // Allow port reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    // Configure server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind and listen for incoming connections

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Server running on port " << port << std::endl;
    REDIS_LOG(INFO, "Server running on port %s", std::to_string(port).c_str());

    while (true) {
        // Blocking call: wait for a client to connect
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("accept");
            continue;
        }

        // Send welcome message to client
        std::string welcome = "Connection established, enter exit to quit\n";
        send(new_socket, welcome.c_str(), welcome.size(), 0);
        REDIS_LOG(INFO, "Client connected");

        std::string recv_buffer;
        bool connection_open = true;

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
        while (connection_open) {
            char buffer[1024];
            int valread = read(new_socket, buffer, sizeof(buffer));

            // Handle client disconnect
            if (valread <= 0) {
                std::cout << "Client disconnected" << std::endl;
                REDIS_LOG(WARN, "Client disconnected");
                break;
            }

            recv_buffer.append(buffer, valread);

            size_t newline_pos;
            while ((newline_pos = recv_buffer.find('\n')) != std::string::npos) {
                std::string command = recv_buffer.substr(0, newline_pos);
                recv_buffer.erase(0, newline_pos + 1);

                if (!command.empty() && command.back() == '\r') {
                    command.pop_back();
                }
                if (command.empty()) {
                    continue;
                }

                // Exit command handling
                if (command == "exit") {
                    std::cout << "Received exit command, closing client connection" << std::endl;
                    REDIS_LOG(INFO, "Received exit command, closing client connection");
                    connection_open = false;
                    break;
                }

                REDIS_LOG(INFO, "Received command %s", command.c_str());
                // Compact AOF (Append Only File)
                std::string response = "Unknown Command\n";
                if (command == "compact") {
                    if (store.redisCompactAof()) {
                        response = "Compact Failed\n";
                        REDIS_LOG(ERROR, "Compact command could not be completed");
                    } else {
                        response = "Compact Successful\n";
                        REDIS_LOG(INFO, "Compact command completed");
                    }
                } else {
                    // Process command and generate response
                    RedisStatus status = processCommand(command, store, response);
                    if (status == REDIS_STATUS_OK) {
                        // response already contains GET result or other success message
                    } else if (status == REDIS_STATUS_NOT_FOUND) {
                        response = "ENTRY NOT FOUND\n";
                    } else {
                        response = "ERROR\n";
                    }
                }

                // Send response back to client
                send(new_socket, response.c_str(), response.size(), 0);
            }
        }

        close(new_socket);
        REDIS_LOG(INFO, "Closed client connection, waiting for next client");
    }

    close(server_fd);
    return 0;
}