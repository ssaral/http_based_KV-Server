cmake_minimum_required(VERSION 3.10)
project(KVServer)

# --- Compiler setup ---
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add optimization and threading flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pthread")

# --- Output directories ---
# Place binaries in build/bin and libraries in build/lib
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Find required packages ---
find_package(PostgreSQL REQUIRED)

# --- Include directories ---
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${PostgreSQL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/third_party/cpp-httplib
    ${CMAKE_SOURCE_DIR}/third_party/json
)

# --- Server executable ---
add_executable(kv_server
    src/server.cpp
    src/cache.cpp
    src/database.cpp
    src/request_handler.cpp
    src/thread_pool.cpp
)

target_link_libraries(kv_server
    ${PostgreSQL_LIBRARIES}
    pthread
)

# --- Load generator executable ---
add_executable(load_generator
    client/load_generator.cpp
)

target_link_libraries(load_generator
    pthread
)

# --- Optional: Show summary info ---
message(STATUS "PostgreSQL include dirs: ${PostgreSQL_INCLUDE_DIRS}")
message(STATUS "PostgreSQL libraries: ${PostgreSQL_LIBRARIES}")
message(STATUS "Executables will be in: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")


# cmake_minimum_required(VERSION 3.10)
# project(KVServer)

# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -O2")

# # Find required packages
# find_package(PostgreSQL REQUIRED)

# # Include directories
# include_directories(${CMAKE_SOURCE_DIR}/src)
# include_directories(${CMAKE_SOURCE_DIR}/include)
# include_directories(${PostgreSQL_INCLUDE_DIRS})

# # Add cpp-httplib (header-only library)
# include_directories(${CMAKE_SOURCE_DIR}/third_party/cpp-httplib)

# # Add nlohmann/json (header-only library)
# include_directories(${CMAKE_SOURCE_DIR}/third_party/json)

# # Server executable
# add_executable(kv_server
#     src/server.cpp
#     src/cache.cpp
#     src/database.cpp
#     src/request_handler.cpp
#     src/thread_pool.cpp
# )

# target_link_libraries(kv_server
#     ${PostgreSQL_LIBRARIES}
#     pthread
# )

# # Load generator executable
# add_executable(load_generator
#     client/load_generator.cpp
# )

# target_link_libraries(load_generator
#     pthread
# )