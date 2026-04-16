//
// Created by Rahul Ranjan on 10/04/26.
//

#ifndef MINI_REDIS_REDISCOMMON_H
#define MINI_REDIS_REDISCOMMON_H
#include "commonLibsEnums.h"

class Logger {
private:
    FILE* general_fp;
    FILE* error_fp;
    std::mutex mtx;

    Logger();   // constructor
    ~Logger();  // destructor

    static const char* level_to_string(RedisLogLevel level);

    static std::string current_time();

public:
    // Disable copy
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Singleton access
    static Logger& instance();

    // 👇 updated signature
    void log(RedisLogLevel level,
             const char* file,
             int line,
             const char* func,
             const char* fmt, ...);
};

// ✅ Macro now injects metadata
#define REDIS_LOG(level, fmt, ...) \
Logger::instance().log(level, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#endif //MINI_REDIS_REDISCOMMON_H
