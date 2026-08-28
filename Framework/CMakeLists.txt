# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Framework)

if(USE_OBJDET)
    add_subdirectory("AI")
endif()
add_subdirectory("Graphics")
add_subdirectory("Video")
add_subdirectory("Util")
add_subdirectory("Window")
