# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Render)

# Include
target_include_directories(
    SV3D
    PUBLIC
    ./include
)

# Sources
set(SRC_DIR ./source)
target_sources(
    SV3D
    PRIVATE
    ${SRC_DIR}/Gains.cpp
    ${SRC_DIR}/RenderApp.cpp
)

if(USE_OBJDET)
    target_sources(
        SV3D
        PRIVATE
        ${SRC_DIR}/ObjectDetection.cpp
    )
endif()