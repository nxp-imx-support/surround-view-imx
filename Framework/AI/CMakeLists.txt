# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(AI)

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
    ${SRC}/DelegateProviders.cpp
    ${SRC}/ExternalDelegateProvider.cpp
    ${SRC}/Tensorflow.cpp
)

find_package(PkgConfig REQUIRED)
pkg_search_module(tflite REQUIRED IMPORTED_TARGET tensorflow2-lite)
target_link_libraries(
    SV3D
    PkgConfig::tflite
)