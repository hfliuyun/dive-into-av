cmake_minimum_required(VERSION 3.15)
project(mini_ffmpeg_student)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)
pkg_check_modules(AVFILTER REQUIRED libavfilter)
pkg_check_modules(SWRESAMPLE REQUIRED libswresample)

# ASAN for Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
endif()

add_executable(mini_ffmpeg 
    main.cpp
    cmd_probe.cpp
    cmd_remux.cpp
    cmd_transcode.cpp
    cmd_resample.cpp
)

target_include_directories(mini_ffmpeg PRIVATE 
    ${AVFORMAT_INCLUDE_DIRS}
    ${AVCODEC_INCLUDE_DIRS}
    ${AVUTIL_INCLUDE_DIRS}
    ${AVFILTER_INCLUDE_DIRS}
    ${SWRESAMPLE_INCLUDE_DIRS}
)

target_link_directories(mini_ffmpeg PRIVATE
    ${AVFORMAT_LIBRARY_DIRS}
    ${AVCODEC_LIBRARY_DIRS}
    ${AVUTIL_LIBRARY_DIRS}
    ${AVFILTER_LIBRARY_DIRS}
    ${SWRESAMPLE_LIBRARY_DIRS}
)

target_link_libraries(mini_ffmpeg PRIVATE 
    ${AVFORMAT_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${AVFILTER_LIBRARIES}
    ${SWRESAMPLE_LIBRARIES}
)
