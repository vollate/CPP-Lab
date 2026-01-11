cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(LF-Structure)

enable_testing()
include(FetchContent)

add_subdirectory(lf_queue)
