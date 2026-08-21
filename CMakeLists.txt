cmake_minimum_required(VERSION 3.16)
project(unitree_cpp_linux_30days LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_subdirectory(week1-cpp-basics/day02-smart-ptr)
add_subdirectory(week1-cpp-basics/day03-dynarray)
