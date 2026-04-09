//
// Created by Rahul Ranjan on 10/04/26.
//
#include <iostream>
#include <fstream>
namespace RedisLogger{
    /**
     * @brief Log Replay Events When redis is loadedx
     * @param message
     */
    void log_replay_event(const std::string& message) {
        std::ofstream log_file("redis.log", std::ios::app);
        if (!log_file.is_open()) {
            return;
        }

        std::time_t now = std::time(nullptr);
        char timestamp[32];
        if (std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now))) {
            log_file << "[" << timestamp << "] ";
        }
        log_file << message << "\n";
    }
}