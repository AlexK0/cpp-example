cmake_minimum_required(VERSION 3.16)
project(example)

set(CMAKE_CXX_STANDARD 17)

add_compile_options(-O3 -fsanitize=address -march=native -Wall -Wextra)
add_executable(example main.cpp)

target_link_libraries(example -fsanitize=address)