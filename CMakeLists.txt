cmake_minimum_required(VERSION 3.16)

project(Cardiac-Monitoring-System VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compilers based on cross-compilation
if(NOT CMAKE_CROSSCOMPILING)
  set(CMAKE_C_COMPILER gcc)
  set(CMAKE_CXX_COMPILER g++)
endif()

# Build type
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Debug build: LOG_DEBUG enabled")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Release build: LOG_DEBUG disabled")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CROSSCOMPILING)
  message(STATUS "Cross-compiling for Raspberry Pi 3")
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Base sources
set(SOURCES
  src/main.cpp
  src/application.cpp
  src/ecg_analyzer.cpp
  src/ecg_filter.cpp 
  src/file_manager.cpp
)

# Add platform-specific sources
if(CMAKE_CROSSCOMPILING)
  list(APPEND SOURCES
    src/ads1115.cpp
    src/sensor_data.cpp
  )
else()
  list(APPEND SOURCES
    src/file_data.cpp
  )
endif()

set(WIRINGPI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/wiring-pi)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${WIRINGPI_LIB_DIR}/include
)

# Link libraries based on cross-compilation
if(CMAKE_CROSSCOMPILING)
  target_link_libraries(${PROJECT_NAME} PRIVATE 
    ${WIRINGPI_LIB_DIR}/wiringpi.a
    m
    pthread
  )
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE 
    m
    pthread
  )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
  $<$<CONFIG:MinSizeRel>:NDEBUG>
  $<$<CONFIG:RelWithDebInfo>:NDEBUG>
)

# Define compile-time macros for data source detection
if(CMAKE_CROSSCOMPILING)
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HARDWARE_SOURCE)
else()
  target_compile_definitions(${PROJECT_NAME} PRIVATE USE_FILE_SOURCE)
endif()

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")