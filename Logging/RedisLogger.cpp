//
// Created by Rahul Ranjan on 10/04/26.
//
#include <cstdarg>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <strings.h>
#include  "RedisCommon.h"
#include "commonLibsEnums.h"

#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>

std::string log_level_global;


/**
 * ============================================================
 * FUNCTION: redisStatusToString
 * ============================================================
 * Converts RedisStatus enum to its string representation.
 * Used for logging and debugging purposes.
 * ============================================================
 */
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

// Constructor
Logger::Logger() {
    general_fp = fopen("redis.log", "a");
    error_fp   = fopen("redis_error.log", "a");
}

// Destructor
Logger::~Logger() {
    if (general_fp) fclose(general_fp);
    if (error_fp) fclose(error_fp);
}

// Singleton instance
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

// Convert level to string
const char* Logger::level_to_string(RedisLogLevel level) {
    switch (level) {
        case DEBUG4: return "DEBUG4";
        case DEBUG3: return "DEBUG3";
        case DEBUG2: return "DEBUG2";
        case DEBUG:  return "DEBUG";
        case INFO:   return "INFO";
        case WARN:   return "WARN";
        case ERROR:  return "ERROR";
        default:     return "UNKNOWN";
    }
}

static int log_level_rank(RedisLogLevel level) {
    switch (level) {
        case ERROR:  return 0;
        case WARN:   return 1;
        case INFO:   return 2;
        case DEBUG:  return 3;
        case DEBUG2: return 4;
        case DEBUG3: return 5;
        case DEBUG4: return 6;
        default:     return 2; // default to INFO
    }
}

static int parse_log_level(const std::string& level) {
    std::string value = level;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return std::toupper(c);
    });

    if (value == "ERROR") return log_level_rank(ERROR);
    if (value == "WARN" || value == "WARNING") return log_level_rank(WARN);
    if (value == "INFO") return log_level_rank(INFO);
    if (value == "DEBUG") return log_level_rank(DEBUG);
    if (value == "DEBUG2") return log_level_rank(DEBUG2);
    if (value == "DEBUG3") return log_level_rank(DEBUG3);
    if (value == "DEBUG4") return log_level_rank(DEBUG4);
    return log_level_rank(INFO);
}

const char* get_short_file(const char* path) {
    const char* slash1 = strrchr(path, '/');
    const char* slash2 = strrchr(path, '\\'); // Windows support

    const char* last = slash1 > slash2 ? slash1 : slash2;
    return last ? last + 1 : path;
}

// Get current timestamp
std::string Logger::current_time() {
    char buffer[64];
    const std::time_t now = std::time(nullptr);
    const std::tm* tm_info = std::localtime(&now);

    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}

// Main log function
void Logger::log(RedisLogLevel level,
                 const char* file,
                 int line,
                 const char* func,
                 const char* fmt, ...) {
    std::lock_guard<std::mutex> lock(mtx);

    int configured_rank = parse_log_level(log_level_global);
    int message_rank = log_level_rank(level);

    if (message_rank > configured_rank) {
        return; // skip messages more verbose than the configured level
    }

    char message[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    std::string time = current_time();
    const char* level_str = level_to_string(level);

    // 🔥 include file, line, function
    const char* short_file = get_short_file(file);
    if (general_fp) {
        fprintf(general_fp,
                "[%s] [%s] [%s:%d:%s] %s\n",
                time.c_str(),
                level_str,
                short_file,
                line,
                func,
                message);
        fflush(general_fp);
    }

    if (level == ERROR && error_fp) {
        fprintf(error_fp,
                "[%s] [%s] [%s:%d:%s] %s\n",
                time.c_str(),
                level_str,
                short_file,
                line,
                func,
                message);
        fflush(error_fp);
    }
}