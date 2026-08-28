/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <pthread.h>
#include <string>
#include <vector>

#include "Material.hpp"
#include "Mesh.hpp"

class ProgramModel;
struct MeshBuffers;

class Model
{
public:
    Model(void);
    virtual ~Model(void);

    void Draw(ProgramModel* program);

protected:
    std::string GetModelFileName(void);
    static void* Load(void* userData);
    void LoadMeshData();

    bool mIsSceneLoaded = false;
    bool mIsInitialized = false;
    pthread_t mThread = 0;

    std::vector<std::shared_ptr<Material>> mMaterials;
    std::vector<std::shared_ptr<Mesh>> mMeshes;

    std::vector<std::shared_ptr<MeshBuffers>> mTempMeshBuffers;
};
