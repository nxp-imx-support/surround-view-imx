# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Graphics)

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
    ${SRC}/FontRenderer.cpp
    ${SRC}/Mesh.cpp
    ${SRC}/Model.cpp
    ${SRC}/Program.cpp
    ${SRC}/Programs.cpp
    ${SRC}/Texture.cpp
    ${SRC}/TextureBuffer.cpp
    ${SRC}/View.cpp
)

if (USE_VIV)
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/TextureBufferVIV.cpp
    )
endif ()
