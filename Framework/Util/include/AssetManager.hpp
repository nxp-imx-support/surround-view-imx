/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

#ifdef ANDROID
#include <android/asset_manager_jni.h>
#endif

class AssetManager
{
protected:
    static std::string mFilesDirPath;

public:
    static const void* GetBuffer(std::string fileName);
    static std::string GetPath(std::string fileName);
    static bool PathExists(std::string path);
    static void MakeDirectory(std::string dirPath);

#ifdef ANDROID
protected:
    static AAssetManager* mAndroidAssetManager;

public:
    static void SetAndroidAssetManager(AAssetManager* aAssetManager, std::string filesDirPath);
#endif
};
