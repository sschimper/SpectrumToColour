cmake_minimum_required(VERSION 3.15)
project(Assignment1 C)

set(CMAKE_C_STANDARD 99)

add_executable(spectocol main.c)

target_link_libraries(spectocol m)
