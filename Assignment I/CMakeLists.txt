cmake_minimum_required(VERSION 3.24.0)

project(assignment1 C)

add_executable(assignment1 libcoro.c solution.c)

target_link_libraries(assignment1 m)