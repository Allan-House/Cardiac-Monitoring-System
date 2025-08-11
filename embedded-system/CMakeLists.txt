cmake_minimum_required(VERSION 3.16)

# Project metadata
project(Cardiac-Monitoring-System VERSION 1.0.0 LANGUAGES CXX)

# C++ standard settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Cross-compilation setup for Raspberry Pi 3
if(CMAKE_CROSSCOMPILING)
    message(STATUS "Cross-compiling for Raspberry Pi 3")
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source files
set(SOURCES
src/main.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Header files
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -O2
)

# Print build information
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")
