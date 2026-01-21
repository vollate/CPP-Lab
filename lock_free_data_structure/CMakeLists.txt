cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(LF-Data-Structure)

if(WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(VCPKG_TRIPLET "x64-windows")
  else()
    set(VCPKG_TRIPLET "x86-windows")
  endif()
elseif(APPLE)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(VCPKG_TRIPLET "arm64-osx")
  else()
    set(VCPKG_TRIPLET "x64-osx")
  endif()
elseif(UNIX)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(VCPKG_TRIPLET "x64-linux")
  else()
    set(VCPKG_TRIPLET "x86-linux")
  endif()
endif()

set(CMAKE_PREFIX_PATH
    "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TRIPLET}")

set(CXX_COMPILE_OPTIONS
    "-Wall -Wextra -Wpedantic -Werror -fsanitize=thread -fsanitize-address-use-after-scope"
)

enable_testing()
find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_subdirectory(lf_queue)
