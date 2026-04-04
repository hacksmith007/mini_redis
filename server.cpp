#include "store.h"
#include <iostream>

// Server implementation
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>
#include "store.h"

std::string processCommand(const std::string&, Store&);

#define PORT 8080

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    Store store;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Server running on port " << PORT << std::endl;

    new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

    char buffer[1024] = {0};

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int valread = read(new_socket, buffer, 1024);

        if (valread <= 0) break;

        std::string response = processCommand(buffer, store);
        response += "\n";

        send(new_socket, response.c_str(), response.size(), 0);
    }

    close(new_socket);
    close(server_fd);

    return 0;
}