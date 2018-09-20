cmake_minimum_required(VERSION 3.10)
project(raptor)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-Wall -O3 -march=native")

set(CATCH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/catch)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

include_directories(include)
include_directories(csa)
add_subdirectory(csa)
add_subdirectory(tests)

OPTION(PROFILE "Profile code" OFF)
if (PROFILE)
    target_compile_definitions(csa_lib PUBLIC -DPROFILE)
    message("Turn on profiling code")
endif (PROFILE)
