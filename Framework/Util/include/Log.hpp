/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#ifdef ANDROID

#include <android/log.h>
#define LOG_TAG "SV3D"
#define LogError(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LogWarning(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#if LOG_DEBUG == 1
#define LogDebug(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LogDebug(...)
#endif
#define LogInfo(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#else

#include <stdio.h>
#define LOG_TAG "SV3D"
#define LogError(...)             \
    printf("ERROR %s ", LOG_TAG); \
    printf(__VA_ARGS__);          \
    printf("\n")
#define LogWarning(...)             \
    printf("WARNING %s ", LOG_TAG); \
    printf(__VA_ARGS__);            \
    printf("\n")
#if LOG_DEBUG == 1
#define LogDebug(...)             \
    printf("DEBUG %s ", LOG_TAG); \
    printf(__VA_ARGS__);          \
    printf("\n")
#else
#define LogDebug(...)
#endif
#define LogInfo(...)             \
    printf("INFO %s ", LOG_TAG); \
    printf(__VA_ARGS__);         \
    printf("\n")

#endif
