# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Util)

# Include
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
    ${SRC}/AssetManager.cpp
    ${SRC}/Compensator.cpp
    ${SRC}/Events.cpp
    ${SRC}/Performances.cpp
    ${SRC}/Settings.cpp
    ${SRC}/Time.cpp
)
