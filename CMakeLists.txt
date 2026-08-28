# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(SV3D)

# CMake modules
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/Tools/CMake")

# Options and arguments
if(NOT DEFINED WINDOWING_SYSTEM)
    message(FATAL_ERROR "WINDOWING_SYSTEM is not set. Set it to Android|FBDev|Wayland|X11")
endif()
message("WINDOWING_SYSTEM is ${WINDOWING_SYSTEM}")

if(NOT DEFINED DEVICE)
    set(DEVICE "unset")
endif()
message("DEVICE is ${DEVICE}")

if (${DEVICE} STREQUAL "IMX8" OR ${DEVICE} STREQUAL "IMX6")
    set (USE_VIV ON CACHE BOOL "Use Vivante extension for OpenGL-ES")
else ()
    set (USE_VIV OFF CACHE BOOL "Use Vivante extension for OpenGL-ES")
endif ()
message("USE_VIV is ${USE_VIV}")

option(USE_OBJDET "Use Object Detection with Tensorflow" ON)
option(USE_DPU_DEWARP "Use G2D DPU dewarp instead of Surround-View dewarp" OFF)

# Default input used in Settings.xml (camera, video, image)
if(NOT DEFINED SETTINGS_DEFAULT_SRC)
    set(SETTINGS_DEFAULT_SRC "camera")
endif()
option(SETTINGS_GENERATE_ALL "Generate Settings.xml for all configuration" ON)

# Global variables
set(BINARY_NAME "SV3D-${WINDOWING_SYSTEM}")
set(INSTALL_DIRECTORY ${PROJECT_NAME})
set(CONTENT_DIR ${PROJECT_SOURCE_DIR}/Content)
set(THIRD_PARTY_DIR ${PROJECT_SOURCE_DIR}/ThirdParty)

# SV3D target
if(${WINDOWING_SYSTEM} STREQUAL "Android")
    add_library(SV3D SHARED "")
else()
    add_executable(SV3D)
endif()
set_target_properties(SV3D PROPERTIES OUTPUT_NAME ${BINARY_NAME})

# Dependencies
if(${WINDOWING_SYSTEM} STREQUAL "Android")
    set(FORWARD_CMAKE_PARAM
        -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
        -DANDROID_NDK=${ANDROID_NDK}
        -DANDROID_API_LEVEL=${ANDROID_API_LEVEL}
        -DANDROID_ABI=${ANDROID_ABI}
        -DANDROID_NDK_VERSION=${ANDROID_NDK_VERSION})
else()
    set(FORWARD_CMAKE_PARAM "")
endif()
message("Forwarding parameters: ${FORWARD_CMAKE_PARAM}")

# Assimp
find_package(Assimp REQUIRED)
target_link_libraries(SV3D Assimp::Assimp)

# Thread (pthread)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(SV3D Threads::Threads)

# OpenCV
find_package(OpenCV REQUIRED)
target_link_libraries(SV3D OpenCV::OpenCV)

# ZLIB
find_package(ZLIB REQUIRED)
target_link_libraries(SV3D ZLIB::ZLIB)

# LibXml2
find_package(LibXml2 REQUIRED)
target_link_libraries(SV3D LibXml2::LibXml2)

# GLM
find_package(GLM REQUIRED)
target_link_libraries(SV3D GLM::GLM)

# OpenGLES
find_package(OpenGLES REQUIRED)
target_link_libraries(SV3D OpenGLES::OpenGLES)

# Vivante GLES extension
if (USE_VIV)
    target_compile_definitions(SV3D PUBLIC USE_VIV=1)
endif ()

if(USE_OBJDET)
    target_compile_definitions(SV3D PUBLIC USE_OBJDET=1)
endif()

# Windowing system
if (${WINDOWING_SYSTEM} STREQUAL "X11")
    target_compile_definitions(SV3D PUBLIC SV3D_X11)
    target_compile_definitions(SV3D PUBLIC EGL_API_X)
    target_link_libraries(SV3D -lX11)
elseif (${WINDOWING_SYSTEM} STREQUAL "Wayland")
    target_compile_definitions(SV3D PUBLIC SV3D_WAYLAND)
    target_compile_definitions(SV3D PUBLIC WL_EGL_PLATFORM)
    target_link_libraries(SV3D -lwayland-client -lwayland-cursor -lwayland-egl)
    target_link_libraries(SV3D -lxkbcommon)
elseif (${WINDOWING_SYSTEM} STREQUAL "Android")
    target_compile_definitions(SV3D PUBLIC ANDROID)
    target_compile_definitions(SV3D PUBLIC __ANDROID__)
    target_compile_definitions(SV3D PUBLIC EGL_API_ANDROID)
else ()
    target_compile_definitions(SV3D PUBLIC EGL_API_FB)
endif ()

# Device
target_compile_definitions(SV3D PUBLIC ${DEVICE})

# Install executable and content
if (${WINDOWING_SYSTEM} STREQUAL "Android")
    set(ANDROID_OUTPUT ${CMAKE_BINARY_DIR}/Android)
    set(ANDROID_MAIN "app/src/main")
    set(ANDROID_OUTPUT_MAIN ${ANDROID_OUTPUT}/${ANDROID_MAIN})
else ()
    install(DIRECTORY ${CONTENT_DIR} DESTINATION ${INSTALL_DIRECTORY})
    install(TARGETS SV3D DESTINATION ${INSTALL_DIRECTORY})
endif()

# Applications and Framework
add_subdirectory("App")
add_subdirectory("Framework")
if(${WINDOWING_SYSTEM} STREQUAL "Android")
    add_subdirectory("GUI/Android")
endif()
add_subdirectory("Presets")
