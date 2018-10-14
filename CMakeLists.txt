cmake_minimum_required(VERSION 3.10)
project(csa)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -pedantic -Wno-maybe-uninitialized -O3")

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

include_directories(include)
include_directories(csa)
add_subdirectory(csa)

OPTION(PROFILE "Profile code" OFF)
if (PROFILE)
    target_compile_definitions(csa_lib PUBLIC -DPROFILE)
    message("Turn on profiling code")
endif (PROFILE)
