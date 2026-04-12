//
// Created by Rahul Ranjan on 10/04/26.
//

#ifndef MINI_REDIS_REDISCOMMON_H
#define MINI_REDIS_REDISCOMMON_H
#include "commonLibsEnums.h"

void redis_logger(const std::string& message,
                      RedisLogLevel level,
                      const char* file,
                      const char* func,
                      int line);

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define REDIS_LOG(level, msg) redis_logger(msg, RedisLogLevel::level, FILENAME, __func__, __LINE__)

#endif //MINI_REDIS_REDISCOMMON_H
