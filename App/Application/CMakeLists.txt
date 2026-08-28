# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Application)

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
    ${SRC}/Application.cpp
    ${SRC}/Applications.cpp
)

# Main file
if(${WINDOWING_SYSTEM} STREQUAL "Android")
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/MainAndroid.cpp
    )
else()
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/MainLinux.cpp
    )
endif()
