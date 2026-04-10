//
// Created by Rahul Ranjan on 10/04/26.
//
#include <iostream>
#include <fstream>
#include "commonLibsEnums.h"


std::string redisStatusToString(const RedisStatus status) {
    switch (status) {
        case REDIS_STATUS_OK:
            return "REDIS_STATUS_OK";
        case REDIS_STATUS_NOT_FOUND:
            return "REDIS_STATUS_NOT_FOUND";
        case REDIS_STATUS_NOT_IMPLEMENTED:
            return "REDIS_STATUS_NOT_IMPLEMENTED";
        case REDIS_STATUS_FAILURE:
            return "REDIS_STATUS_FAILURE";
        case REDIS_STATUS_NOT_SUPPORTED:
            return "REDIS_STATUS_NOT_SUPPORTED";
        case REDIS_STATUS_INVALID_ARGUMENT:
            return "REDIS_STATUS_INVALID_ARGUMENT";
        default:
            return "REDIS_STATUS_UNKNOWN";
    }
}

std::string redisLogLevelToString(const RedisLogLevel level) {
    switch (level) {
        case INFO:
            return "INFO";
        case DEBUG:
            return "DEBUG";
        case WARNING:
            return "WARN";
        case ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Logger Function
 * @param message
 * @param level
 * @param file
 * @param func
 */
void redis_logger(const std::string& message,
                  RedisLogLevel level,
                  const char* file,
                  const char* func,
                  int line) {

    std::ofstream log_file("redis.log", std::ios::app);
    std::ofstream err_log_file("redis_error.log", std::ios::app);
    if (!log_file.is_open() || !err_log_file.is_open()) {
        return;
    }

    std::time_t now = std::time(nullptr);
    char timestamp[32];
    if (std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now))) {
        log_file << "[" << timestamp << "] ";
    }
    if (level == ERROR) {
        err_log_file  << "[" << timestamp << "] " << "[" << redisLogLevelToString(level) << "] "
             << "[" << file << ": " << line << " - " << func << "] "
                << message << "\n";
    }
    log_file << "[" << redisLogLevelToString(level) << "] "
             << "[" << file << ": " << line << " - " << func << "] "
                << message << "\n";
}