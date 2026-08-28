# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Calibration)

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
    ${SRC_DIR}/CalibrationApp.cpp
    ${SRC_DIR}/Camera.cpp
    ${SRC_DIR}/CameraModel.cpp
    ${SRC_DIR}/Contours.cpp
    ${SRC_DIR}/FisheyeModel.cpp
    ${SRC_DIR}/Grid.cpp
    ${SRC_DIR}/Masks.cpp
    ${SRC_DIR}/RectilinearModel.cpp
)

