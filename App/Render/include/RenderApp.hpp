/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Application.hpp"
#include "Gains.hpp"
#include "Mesh.hpp"
#include "Model.hpp"
#ifdef USE_OBJDET
#include "ObjectDetection.hpp"
#endif
#include "Performances.hpp"
#include "Programs.hpp"
#include "View.hpp"

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class RenderApp : public Application
{
public:
    RenderApp(std::shared_ptr<Settings> settings, std::shared_ptr<View> view, std::shared_ptr<Performances> performances);
    virtual ~RenderApp();

public:
    // Inherited from Application
    virtual void Update(std::vector<std::shared_ptr<Texture>> videoTextures);
    // Inherited from Events
    virtual void OnEvent(const Event& inEvent);

protected:
    void InitEcTextures(void);

    void InitMatrices();
    void Render(std::vector<std::shared_ptr<Texture>> textures);

    void DeInitEcTextures(void);

protected:
    uint32_t mWidth;
    uint32_t mHeight;

    std::vector<std::shared_ptr<Mesh>> mOverlapMeshes;
    std::vector<std::shared_ptr<Mesh>> mNonOverlapMeshes;

    ProgramGainMask mOverlapProgram;
    ProgramGainMaskExt mOverlapExtProgram;
    ProgramGain mNonOverlapProgram;
    ProgramGainExt mNonOverlapExtProgram;

    std::vector<std::shared_ptr<Texture>> mMaskTextures;

    // Exposure correction
    int mEcFrameCnt = 0;
    std::vector<std::shared_ptr<Mesh>> mExposureCorrectionMeshes;
    ProgramTexturedQuad mExposureCorrectionProgram;
    ProgramTexturedQuadExt mExposureCorrectionExtProgram;
    GLuint mEcFbo;
    GLuint mEcRbo;
    std::unique_ptr<Gains> mGain = nullptr;

    // Car Model
    glm::mat4 mCarProjection;
    glm::mat4 mCarRotationMatrix;
    glm::mat4 mCarModelMatrix;
    float rx = 0.0f, ry = 0.0f, px = 0.0f, py = 0.0f, pz = -5.0f;

    Model mCarModel;
    ProgramModel mCarModelProgram;

    // Object Detection
#ifdef USE_OBJDET
    std::shared_ptr<ObjectDetection> mObjectDetection;
#endif
};
