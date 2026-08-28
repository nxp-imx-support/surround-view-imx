# Copyright 2026 NXP
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.13)

project(Android)

# android lib
target_link_libraries(SV3D android)

# Log_print library
find_library(log-lib log)
target_link_libraries(SV3D ${log-lib})
target_link_libraries(SV3D -lmediandk)
target_link_libraries(SV3D -lnativehelper)

# Input variables needed to configure gradle
if(NOT DEFINED ANDROID_API_LEVEL)
    message(FATAL_ERROR "ANDROID_API_LEVEL must be defined, e.g. 30")
elseif(NOT DEFINED ANDROID_NDK_VERSION)
    message(FATAL_ERROR "ANDROID_NDK_VERSION must be defined, e.g. 23.0.7599858")
elseif(NOT DEFINED ANDROID_ABI)
    message(FATAL_ERROR "ANDROID_ABI must be defined, e.g. arm64-v8a")
endif()
message("ANDROID_API_LEVEL is ${ANDROID_API_LEVEL}")
message("ANDROID_NDK_VERSION is ${ANDROID_NDK_VERSION}")
message("ANDROID_ABI is ${ANDROID_ABI}")

# Android input/output directories
set(ANDROID_INPUT_MAIN ${CMAKE_CURRENT_SOURCE_DIR}/${ANDROID_MAIN})
set(ANDROID_INPUT_JAVA ${ANDROID_INPUT_MAIN}/java/com/nxp/sv3d)
set(ANDROID_OUTPUT_JAVA ${ANDROID_OUTPUT_MAIN}/java/com/nxp/sv3d)

# Copy Android directory and resources files
file(COPY ${CMAKE_CURRENT_SOURCE_DIR} DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CONTENT_DIR} DESTINATION ${ANDROID_OUTPUT_MAIN}/assets)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/app/build.gradle
    ${ANDROID_OUTPUT}/app/build.gradle
    @ONLY
)

# Configure @BINARY_NAME@ in SV3D activity
configure_file(${ANDROID_INPUT_JAVA}/SV3DActivity.java ${ANDROID_OUTPUT_JAVA}/SV3DActivity.java @ONLY)
configure_file(${ANDROID_INPUT_JAVA}/SV3DApp.java ${ANDROID_OUTPUT_JAVA}/SV3DApp.java COPYONLY)
configure_file(${ANDROID_INPUT_JAVA}/SV3DNative.java ${ANDROID_OUTPUT_JAVA}/SV3DNative.java COPYONLY)
configure_file(${ANDROID_INPUT_JAVA}/SV3DRenderer.java ${ANDROID_OUTPUT_JAVA}/SV3DRenderer.java COPYONLY)
configure_file(${ANDROID_INPUT_JAVA}/SV3DSurfaceView.java ${ANDROID_OUTPUT_JAVA}/SV3DSurfaceView.java COPYONLY)

# Files dependencies for gradle automatic rebuild
set(JAVA_FILES 
    "${ANDROID_INPUT_JAVA}/SV3DActivity.java"
    "${ANDROID_INPUT_JAVA}/SV3DApp.java"
    "${ANDROID_INPUT_JAVA}/SV3DNative.java"
    "${ANDROID_INPUT_JAVA}/SV3DRenderer.java"
    "${ANDROID_INPUT_JAVA}/SV3DSurfaceView.java"
)

# APK output location
set(APK_FILE ${CMAKE_BINARY_DIR}/com.nxp.sv3d.apk)

# Build java and create apk using gradle
add_custom_command(OUTPUT ${APK_FILE}
    DEPENDS SV3D ${JAVA_FILES}
    WORKING_DIRECTORY ${ANDROID_OUTPUT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${ANDROID_OUTPUT_MAIN}/jniLibs/${ANDROID_ABI}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:SV3D> ${ANDROID_OUTPUT_MAIN}/jniLibs/${ANDROID_ABI}/$<TARGET_FILE_NAME:SV3D>
    COMMAND ./gradlew clean assembleDebug
    COMMAND ${CMAKE_COMMAND} -E copy ${ANDROID_OUTPUT}/app/build/outputs/apk/debug/app-debug.apk ${APK_FILE}
)

install(FILES ${APK_FILE} DESTINATION ${INSTALL_DIRECTORY})

add_custom_target(APK ALL DEPENDS ${APK_FILE})
