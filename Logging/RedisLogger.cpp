//
// Created by Rahul Ranjan on 10/04/26.
//
#include <cstdarg>
#include <thread>
#include <mutex>
#include  "RedisCommon.h"
#include "commonLibsEnums.h"

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
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        default:    return "UNKNOWN";
    }
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