/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Application.hpp"
#include "Camera.hpp"
#include "CameraModel.hpp"
#include "Compensator.hpp"
#include "Grid.hpp"
#include "Masks.hpp"
#include "Mesh.hpp"
#include "Programs.hpp"
#include "View.hpp"

class CalibrationApp : public Application
{
public:
    CalibrationApp(std::shared_ptr<Settings> settings, std::shared_ptr<View> view);
    virtual ~CalibrationApp() = default;

    // Inherited from Application
    virtual void Update(std::vector<std::shared_ptr<Texture>> videoTextures);
    // Inherited from Events
    virtual void OnEvent(const Event& inEvent);

protected:
    enum ViewState
    {
        Invalid = -1,
        Fisheye = 0,
        Defisheye,
        Contours,
        Grids,
        Result
    };

    void Process(std::vector<std::shared_ptr<Texture>> videoTextures);
    void Render(std::vector<std::shared_ptr<Texture>> videoTextures);
    void PrintState();

    bool GetDefisheye();
    bool SearchContours(uint index, std::shared_ptr<Texture> cameraTexture);
    bool GetContours(std::vector<std::shared_ptr<Texture>> videoTextures);
    bool GetGrids();
    void SaveGrids(void);

protected:
    ViewState mState = ViewState::Invalid;
    ViewState mStateRequested = ViewState::Fisheye;

    int mTemplateWidth;
    int mTemplateHeight;

    std::vector<std::shared_ptr<Camera>> mCameras;
    std::vector<std::shared_ptr<CurvilinearGrid>> mGrids;
    std::vector<std::shared_ptr<Mesh>> mCornerMeshes;
    std::vector<std::shared_ptr<Mesh>> mOverlapMeshes;
    std::vector<std::shared_ptr<Mesh>> mNonOverlapMeshes;
    std::vector<std::shared_ptr<Mesh>> mContoursMeshes;
    std::vector<std::shared_ptr<Mesh>> mGridMeshes;
    std::vector<std::shared_ptr<CameraModel>> mCameraModels;

    ProgramTexturedQuad mProgramQuad;
    ProgramTexturedQuadExt mProgramQuadExt;
    ProgramLine mProgramLine;
    ProgramGain mProgram;
    ProgramGainExt mProgramExt;
};
