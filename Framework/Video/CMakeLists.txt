# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Video)

# Include directory
target_include_directories(
    SV3D
    PUBLIC
    ./include
)

# Sources
set(SRC ./source)
target_sources(
    SV3D
    PRIVATE
    ${SRC}/CameraV4L2.cpp
    ${SRC}/ImageCV.cpp
    ${SRC}/VideoCV.cpp
    ${SRC}/VideoStream.cpp
    ${SRC}/VideoStreams.cpp
)

# GStreamer
if(${WINDOWING_SYSTEM} STREQUAL "Android")
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/Camera2.cpp
    )
    target_link_libraries(SV3D -lcamera2ndk)
else()
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/GstPlayer.cpp
    )

    find_package(PkgConfig REQUIRED)
    pkg_search_module(gstreamer REQUIRED IMPORTED_TARGET gstreamer-1.0)
    pkg_search_module(gstreamer-gl REQUIRED IMPORTED_TARGET gstreamer-gl-1.0)
    target_link_libraries(
        SV3D
        PkgConfig::gstreamer
        PkgConfig::gstreamer-gl
    )
endif()
