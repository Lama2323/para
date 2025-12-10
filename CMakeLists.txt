cmake_minimum_required(VERSION 3.14)
project(GameServerSimulation VERSION 1.0.0 LANGUAGES CXX)

# C++17 required for std::optional and other features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /O2)
else()
    add_compile_options(-Wall -Wextra -O2 -pthread)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/game/match.cpp
    src/game/game_server.cpp
    src/client/client.cpp
)

# Header files (for IDE)
set(HEADERS
    src/common/types.hpp
    src/common/data_structures.hpp
    src/scheduler/work_stealing_queue.hpp
    src/scheduler/thread_pool.hpp
    src/game/match.hpp
    src/game/game_server.hpp
    src/client/client.hpp
)

# Main executable
add_executable(game_server ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(game_server PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link pthread on Unix
if(NOT MSVC)
    find_package(Threads REQUIRED)
    target_link_libraries(game_server PRIVATE Threads::Threads)
endif()

# Output directory
set_target_properties(game_server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install target
install(TARGETS game_server
    RUNTIME DESTINATION bin
)

# Print configuration
message(STATUS "")
message(STATUS "=== Game Server Simulation ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")