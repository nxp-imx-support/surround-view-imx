/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Application.hpp"
#include "Mesh.hpp"
#include "Programs.hpp"
#include "View.hpp"

class CapturingApp : public Application
{
public:
    CapturingApp(std::shared_ptr<Settings> settings, std::shared_ptr<View> view, int cameraNum = -1);
    virtual ~CapturingApp();

    // Inherited from Application
    virtual void Update(std::vector<std::shared_ptr<Texture>> videoTextures);
    // Inherited from Events
    virtual void OnEvent(const Event& inEvent);

protected:
    struct CameraIndices
    {
        int ParamIndex;
        int ViewIndex;
        int CurrentFrame;
    };

    int mCurrentCamera;
    int mSingleCameraNum;
    std::vector<CameraIndices> mCameraIndices;
    ProgramTexturedQuad mProgram;
    ProgramTexturedQuadExt mProgramExt;
    bool mCaptureRequested = false;
};
