cmake_minimum_required(VERSION 3.15)
project(lesson19_sdl_render)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL3
# SDL3 provides a CMake config file that handles include paths and linking correctly.
find_package(SDL3 REQUIRED)

# ASAN for Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

add_executable(lesson19_sdl_render main.cpp)

# Use the exported target SDL3::SDL3
target_link_libraries(lesson19_sdl_render PRIVATE SDL3::SDL3)
