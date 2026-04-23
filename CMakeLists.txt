cmake_minimum_required(VERSION 3.22)
project(RTSP LANGUAGES CXX CUDA)

if(MSVC)
   add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/bigobj>)
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets DBus)
pkg_check_modules(RNNOISE REQUIRED rnnoise)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER_APP REQUIRED gstreamer-app-1.0)
pkg_check_modules(GSTREAMER_RTSP REQUIRED gstreamer-rtsp-server-1.0)
pkg_check_modules(GSTREAMER_VIDEO REQUIRED gstreamer-video-1.0)
pkg_check_modules(FFMPEG REQUIRED
    libavformat
    libavcodec
    libavfilter
    libavdevice
    libavutil
    libswscale
    libswresample
)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_subdirectory(${SRC_DIR}/LibBASE)
add_subdirectory(${SRC_DIR}/LibVIS)
add_subdirectory(${SRC_DIR}/LibAUDIO)
add_subdirectory(${SRC_DIR}/LibRTSP)
add_subdirectory(${SRC_DIR}/LibSMOKE)
add_subdirectory(${SRC_DIR}/LibQT)

