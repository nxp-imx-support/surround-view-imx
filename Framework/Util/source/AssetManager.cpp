/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "AssetManager.hpp"
#include "Log.hpp"

#include <sys/stat.h>
#include <sys/types.h>

std::string AssetManager::mFilesDirPath = {};

bool AssetManager::PathExists(std::string path)
{
    struct stat st = { 0 };
    std::string filesDirPath = mFilesDirPath + path;
    return (stat(filesDirPath.c_str(), &st) == 0);
}

void AssetManager::MakeDirectory(std::string dirPath)
{
    if (PathExists(dirPath) == false) {
        std::string filesDirPath = mFilesDirPath + dirPath;
        LogInfo("Creating %s", filesDirPath.c_str());
        mkdir(filesDirPath.c_str(), 0777);
    }
}

#ifdef ANDROID
#include <android/asset_manager.h>
AAssetManager* AssetManager::mAndroidAssetManager = nullptr;
#include "FilesName.hpp"

void AssetManager::SetAndroidAssetManager(AAssetManager* aAssetManager, std::string filesDirPath)
{
    if (aAssetManager == nullptr) {
        LogWarning("SetAndroidAssetManager called with null pointer");
    }
    mAndroidAssetManager = aAssetManager;
    mFilesDirPath = filesDirPath + "/";

    // Create folders hierarchy
    MakeDirectory(CONTENT_DIR);
    MakeDirectory(CAMERA_INPUTS_DIR);
    MakeDirectory(CAMERA_MODELS_DIR);
    MakeDirectory(MESHES_DIR);
    MakeDirectory(MODELS_DIR);
    MakeDirectory(TEMPLATES_DIR);
    MakeDirectory(GENERATED_DIR);
    MakeDirectory(VERTICES_DIR);
    MakeDirectory(COMPENSATOR_DIR);
    MakeDirectory(MASKS_DIR);
    MakeDirectory(CHESSBOARDS_DIR);
    MakeDirectory(CHESSBOARDS_DIR CHESSBOARD_PREFIX "1");
    MakeDirectory(CHESSBOARDS_DIR CHESSBOARD_PREFIX "2");
    MakeDirectory(CHESSBOARDS_DIR CHESSBOARD_PREFIX "3");
    MakeDirectory(CHESSBOARDS_DIR CHESSBOARD_PREFIX "4");
    MakeDirectory(AI_DIR);
}
#endif

const void* AssetManager::GetBuffer(std::string fileName)
{
#ifdef ANDROID
    if (mAndroidAssetManager == nullptr) {
        LogError("Android Asset Manager not initialized");
        return nullptr;
    }
    AAsset* asset = AAssetManager_open(mAndroidAssetManager, fileName.c_str(), AASSET_MODE_RANDOM);
    return AAsset_getBuffer(asset);
#else
    // TODO
    LogError("AssetManager::GetBuffer TODO");
    return nullptr;
#endif
}

std::string AssetManager::GetPath(std::string fileName)
{
    std::string newPath = mFilesDirPath + fileName;
#ifdef ANDROID
    if (mAndroidAssetManager == nullptr) {
        LogError("Android Asset Manager not initialized");
        return nullptr;
    }

    // If file exists in Files directory return path to the file
    if (PathExists(fileName) == false) {
        // Else if file is found in app asset, create file on disk from asset
        AAsset* asset = AAssetManager_open(mAndroidAssetManager, fileName.c_str(), AASSET_MODE_STREAMING);
        if (asset != nullptr) {
            char buf[BUFSIZ];
            int nb_read = 0;
            LogInfo("Importing file %s", newPath.c_str());
            FILE* file = fopen(newPath.c_str(), "w");
            if (file == nullptr) {
                LogError("Cannot create file %s", newPath.c_str());
                return nullptr;
            }
            while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0) {
                fwrite(buf, nb_read, 1, file);
            }
            fclose(file);
            AAsset_close(asset);
        }
        // If asset does not exist return path to Files directory + file name
    }
#endif
    return newPath;
}
