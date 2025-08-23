cmake_minimum_required(VERSION 3.16)

project(TESTS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CROSSCOMPILING)
  message(STATUS "Cross-compiling for Raspberry Pi 3")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include)
set(WIRINGPI_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../libs/wiring-pi)

set(SOURCES
  main.cpp
  ${SRC_DIR}/ads1115.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${INCLUDE_DIR}
  ${WIRINGPI_LIB_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
  ${WIRINGPI_LIB_DIR}/wiringpi.a
  m
  pthread
)

target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  #-O2
)