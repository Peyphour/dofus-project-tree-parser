cmake_minimum_required(VERSION 3.8)
project(project_tree)

set(CMAKE_C_COMPILER_TARGET C99)

set(SOURCE_FILES main.c)
add_executable(project_tree ${SOURCE_FILES})