cmake_minimum_required(VERSION 4.2)
project(mini_redis)

set(CMAKE_CXX_STANDARD 20)

add_executable(mini_redis main.cpp
        store.cpp
        parser.cpp)
add_executable(server server.cpp
        store.cpp
        parser.cpp)
