//
// Created by Rahul Ranjan on 10/04/26.
//

#ifndef MINI_REDIS_COMMONLIBSENUMS_H
#define MINI_REDIS_COMMONLIBSENUMS_H
#include <iostream>
#include <string>

enum RedisLogLevel {
    INFO,
    WARNING,
    ERROR,
    DEBUG,
};

enum RedisStatus {
    REDIS_STATUS_OK = 0,
    REDIS_STATUS_FAILURE = 1,
    REDIS_STATUS_NOT_FOUND = 2,
    REDIS_STATUS_NOT_IMPLEMENTED = 3,
    REDIS_STATUS_NOT_SUPPORTED = 4,
    REDIS_STATUS_INVALID_ARGUMENT = 5,
};
#endif //MINI_REDIS_COMMONLIBSENUMS_H
