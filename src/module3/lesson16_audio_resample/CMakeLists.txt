cmake_minimum_required(VERSION 3.15)
project(lesson16_audio_resample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    message(STATUS "Debug mode: AddressSanitizer enabled")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(SWRESAMPLE REQUIRED libswresample)

message(STATUS "AVCodec version: ${AVCODEC_VERSION}")
message(STATUS "AVFormat version: ${AVFORMAT_VERSION}")
message(STATUS "AVUtil version: ${AVUTIL_VERSION}")
message(STATUS "SWResample version: ${SWRESAMPLE_VERSION}")

add_executable(audio_resample main.cpp)

target_include_directories(audio_resample PRIVATE
    ${AVCODEC_INCLUDE_DIRS}
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${SWRESAMPLE_INCLUDE_DIRS}
)

target_link_libraries(audio_resample
    ${AVCODEC_LIBRARIES}
    ${AVFORMAT_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWRESAMPLE_LIBRARIES}
)

target_link_directories(audio_resample PRIVATE
    ${AVCODEC_LIBRARY_DIRS}
    ${AVFORMAT_LIBRARY_DIRS}
    ${AVUTIL_LIBRARY_DIRS}
    ${SWRESAMPLE_LIBRARY_DIRS}
)
