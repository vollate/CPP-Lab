cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(LF-Data-Structure)

enable_testing()

set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed")

add_subdirectory(lf_queue)
