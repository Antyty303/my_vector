cmake_minimum_required(VERSION 4.2)

project(my_vector)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(my_vector INTERFACE "./include/vector.h")
target_include_directories(my_vector INTERFACE "./include/")

add_executable(test "./tests/main.cpp")
target_link_libraries(test PRIVATE my_vector)

message(STATUS "SUCCESSFUL")