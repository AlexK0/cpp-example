cmake_minimum_required(VERSION 3.16)
project(example)

set(CMAKE_CXX_STANDARD 17)


include_directories(src)
add_executable(example src/main.cpp)
