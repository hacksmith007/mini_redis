//
// Created by Rahul Ranjan on 10/04/26.
//

#ifndef MINI_REDIS_PARSER_H
#define MINI_REDIS_PARSER_H
#include <string>
#include "commonLibsEnums.h"
std::string processCommand(const std::string& input, Store& store);
RedisStatus processCommand(const std::string& input, Store& store, std::string &out);
#endif //MINI_REDIS_PARSER_H
