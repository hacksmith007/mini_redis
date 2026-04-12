cmake_minimum_required(VERSION 4.2)
project(mini_redis)

set(CMAKE_CXX_STANDARD 20)

add_executable(server Server/server.cpp
        Store/store.cpp
        Parser/parser.cpp Logging/RedisLogger.cpp
        Scheduler/scheduler.cpp)
target_include_directories(server PRIVATE ${PROJECT_SOURCE_DIR}/Store/include
                                  ${PROJECT_SOURCE_DIR}/Parser/include
                                  ${PROJECT_SOURCE_DIR}/Server/include
                                  ${PROJECT_SOURCE_DIR}/Scheduler/include
                                  ${PROJECT_SOURCE_DIR}/include)