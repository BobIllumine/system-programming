cmake_minimum_required(VERSION 3.24)
project(Assignment_II C)

set(CMAKE_C_STANDARD 23)

add_library(A_II_Lib STATIC heap_help/heap_help.c
                            cmd/cmd.c
                            regex_lib/regex_lib.c)
target_include_directories(A_II_Lib PUBLIC heap_help cmd regex_lib)
add_executable(Assignment_II main.c)

target_link_libraries(Assignment_II A_II_Lib)
#add_executable(Assignment_II heap_help/heap_help.c regex_lib/regex_lib.c main.c)
#test.c
#main.c