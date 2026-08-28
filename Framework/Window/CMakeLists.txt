# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Window)

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
    ${SRC}/Window.cpp
)

# Wayland
if (${WINDOWING_SYSTEM} STREQUAL "Wayland")
    include(WaylandScanner)
    wayland_scanner(TARGET SV3D PHASE stable PROTOCOL xdg-shell)
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/Wayland/Wayland.cpp
        ${SRC}/Wayland/WaylandSeat.cpp
        ${SRC}/Wayland/WaylandShellXdg.cpp
    )
    target_include_directories(
        SV3D
        PRIVATE
        ${SRC}/Wayland
    )
endif ()

# X11
if (${WINDOWING_SYSTEM} STREQUAL "X11")
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/X11/X11.cpp
    )
endif ()

# Android
if (${WINDOWING_SYSTEM} STREQUAL "Android")
    target_sources(
        SV3D
        PRIVATE
        ${SRC}/Android/Android.cpp
    )
endif ()