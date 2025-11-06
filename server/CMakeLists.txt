cmake_minimum_required(VERSION 3.10)
project(KVServer)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(server src/main.cpp)

# Link PostgreSQL C and C++ libraries manually
target_link_libraries(server pqxx pq)
