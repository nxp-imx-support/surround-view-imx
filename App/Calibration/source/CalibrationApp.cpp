/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CalibrationApp.hpp"

#include "FisheyeModel.hpp"
#include "RectilinearModel.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Texture.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

CalibrationApp::CalibrationApp(std::shared_ptr<Settings> settings, std::shared_ptr<View> view)
    : Application(settings, view)
{
    // Read XML parameters
    if (mSettings->GetTmpMaxVal(TEMPLATE_PREFIX "1.txt", &mTemplateWidth) == -1) {
        LogError("Failed to read template");
    }
    if (mSettings->GetTmpMaxVal(TEMPLATE_PREFIX "2.txt", &mTemplateHeight) == -1) {
        LogError("Failed to read template");
    }

    // Create camera capture
    mCameraCount = mSettings->camerasCount;
    for (int i = 0; i < mCameraCount; i++) {
        // Load corner mesh
        auto cornerMesh = std::make_shared<Mesh>();
        mCornerMeshes.push_back(cornerMesh);

        // Create bowl meshes
        mOverlapMeshes.push_back(std::make_shared<Mesh>());
        mNonOverlapMeshes.push_back(std::make_shared<Mesh>());

        // Create empty meshes for contours and grids
        mContoursMeshes.push_back(std::make_shared<Mesh>());
        mGridMeshes.push_back(std::make_shared<Mesh>());
    }

    // Create camera objects
    for (int i = 0; i < mCameraCount; i++) {
        std::shared_ptr<CameraModel> cameraModel;
        if (mSettings->dewarp) {
            std::string calibResult = mSettings->cameraModels + CALIB_RESULTS_PREFIX + std::to_string(i + 1) + ".txt";
            cameraModel = std::make_shared<FisheyeModel>(calibResult, mSettings->cameras[i].sf);
        } else {
            cameraModel = std::make_shared<RectilinearModel>(mSettings->cameras[i].width, mSettings->cameras[i].height);
        }
        mCameraModels.push_back(cameraModel);

        auto camera = std::make_shared<Camera>(cameraModel, i);
        mCameras.push_back(camera);
    }

    // Create grid objects
    for (int i = 0; i < mCameraCount; i++) {
        auto grid = std::make_shared<CurvilinearGrid>(mSettings->gridAngles, mSettings->gridStartAngle,
            mSettings->gridPointsZCount, mSettings->gridStepX);
        mGrids.push_back(grid);
    }

    LogInfo("Calibration initialisation done");
}

void CalibrationApp::Update(std::vector<std::shared_ptr<Texture>> videoTextures)
{
    Process(videoTextures);
    Render(videoTextures);
}

void CalibrationApp::OnEvent(const Event& inEvent)
{
    // Handle events
    switch (inEvent.Type) {
    case EventType::KeyPress:
        switch (inEvent.KeyValue) {
        case Key::Esc:
            // Quit
            LogInfo("The exit key was pressed");
            RequestQuit();
            break;
        case Key::Right:
            // Next view
            if (mState < ViewState::Result) {
                mStateRequested = (ViewState)((int)mState + 1);
            } else {
                RequestQuit();
            }
            break;
        case Key::Left:
            // Previous view
            if (mState > ViewState::Fisheye) {
                mStateRequested = (ViewState)((int)mState - 1);
            }
            break;
        case Key::F5:
            // Update parameters
            if (mSettings->ReadXML(SETTINGS_PATH_FILE) == -1) {
                break;
            }
            for (uint i = 0U; i < mCameraCount; i++) {
                mCameras[i]->UpdateLUT(mSettings->cameras[i].sf);
            }
            mStateRequested = mState;
            mState = ViewState::Invalid;
            break;
        default:
            break;
        }
        break;
    case EventType::MousePress:
        // Next view
        if (mState < ViewState::Result) {
            mStateRequested = (ViewState)((int)mState + 1);
        } else {
            RequestQuit();
        }
        break;
    default:
        break;
    }
}

void CalibrationApp::Process(std::vector<std::shared_ptr<Texture>> videoTextures)
{
    if (mStateRequested != mState) {
        mState = mStateRequested;
        PrintState();

        switch (mState) {
        case ViewState::Fisheye:
            break;

        case ViewState::Defisheye:
            GetDefisheye();
            break;

        case ViewState::Contours:
            GetContours(videoTextures);
            break;

        case ViewState::Grids:
            GetGrids();
            break;

        case ViewState::Result:
            SaveGrids();
            for (uint i = 0U; i < mCameraCount; i++) {
                mOverlapMeshes[i]->Load(VERTICES_DIR ARRAY_PREFIX + std::to_string(i + 1U) + "1");
                mNonOverlapMeshes[i]->Load(VERTICES_DIR ARRAY_PREFIX + std::to_string(i + 1U) + "2");
            }
            break;

        default:
            LogError("Unknown View State");
            break;
        }
        LogInfo("\tDone");
    }
}

void CalibrationApp::Render(std::vector<std::shared_ptr<Texture>> videoTextures)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    mView->BindFramebuffer(0);

    if (mState != ViewState::Result) {
        for (uint i = 0U; i < mCameraCount; i++) {
            mView->ViewportCorner(i);
            if (auto texture = videoTextures[i]; texture != nullptr) {
                ProgramTexturedQuad& program = texture->IsExternalOES() ? mProgramQuadExt : mProgramQuad;
                program.Use();
                program.SetGain(1.0f);
                program.SetTexture(texture);
                if (mState == ViewState::Fisheye) {
                    mView->RenderQuad();
                } else {
                    mCornerMeshes[i]->Render(RenderPrimitive::Triangle);
                }
            }
        }
    }

    if (mState == ViewState::Contours) {
        mProgramLine.Use();
        for (uint i = 0U; i < mCameraCount; i++) {
            mView->ViewportCorner(i);
            mContoursMeshes[i]->Render(RenderPrimitive::Line);
        }
    }

    if (mState == ViewState::Grids) {
        mProgramLine.Use();
        for (uint i = 0U; i < mCameraCount; i++) {
            mView->ViewportCorner(i);
            mGridMeshes[i]->Render(RenderPrimitive::Point);
        }
    }

    if (mState == ViewState::Result) {
        mView->ViewportFull();
        float gain[4] = { 1.0f, 1.0f, 1.0f, 0.5f };
        mProgram.SetGain(gain);
        for (uint i = 0U; i < mCameraCount; i++) {
            if (auto texture = videoTextures[i]; texture != nullptr) {
                ProgramGain& program = texture->IsExternalOES() ? mProgramExt : mProgram;
                program.Use();
                program.SetTransform(glm::value_ptr(glm::scale(glm::vec3(0.2f, 0.2f, 0.2f))));
                program.SetGain(1.0f);
                program.SetTexture(texture);
                mOverlapMeshes[i]->Render(RenderPrimitive::Triangle);
                mNonOverlapMeshes[i]->Render(RenderPrimitive::Triangle);
            }
        }
    }
}

void CalibrationApp::PrintState()
{
    switch (mState) {
    case ViewState::Fisheye:
        LogInfo("STEP 0: Load fisheye camera view...");
        break;
    case ViewState::Defisheye:
        LogInfo("STEP 1: Remove fisheye distortion...");
        break;
    case ViewState::Contours:
        LogInfo("STEP 2: Search contours...");
        break;
    case ViewState::Grids:
        LogInfo("STEP 3: Prepare meshes...");
        break;
    case ViewState::Result:
        LogInfo("STEP 4: Calculate the result view...");
        break;
    default:
        LogError("Unknown View State");
        break;
    }
}

bool CalibrationApp::GetDefisheye()
{
    for (uint i = 0U; i < mCameraCount; i++) {
        float* data = nullptr;
        int size = mCameras[i]->CreateVertices(&data, 10);
        if (data != nullptr) {
            mCornerMeshes[i]->Load(data, size);
            free(data);
        } else {
            LogError("Creating defisheye view failed");
            return false;
        }
    }
    return true;
}

bool CalibrationApp::SearchContours(uint index, std::shared_ptr<Texture> cameraTexture)
{
    cv::Mat img = cameraTexture->GetData();

    std::string templatePoints = mSettings->templateFiles + TEMPLATE_PREFIX + std::to_string(index + 1U) + ".txt";
    std::string chessboard = CHESSBOARDS_DIR CHESSBOARD_PREFIX + std::to_string(index + 1U) + "/";

    // Set roi in which contours will be searched (in % of image height) and empiric bound for minimal allowed perimeter
    // for contours
    mCameras[index]->SetRoi(mSettings->cameras[index].roi);
    mCameras[index]->SetContourMinSize(mSettings->cameras[index].cntr_min_size);

    // Set template size and reference points
    if (mCameras[index]->SetTemplate(templatePoints, cv::Size2d(mTemplateWidth, mTemplateHeight)) != 0) {
        return false;
    }

    // Calculate intrinsic camera parameters using chessboard image
    std::string chessboard_name = std::string(FRAME_PREFIX + std::to_string(index + 1U) + "_");
    if (mCameras[index]->SetIntrinsic(chessboard, chessboard_name, mSettings->cameras[index].chessboard_num,
            cv::Size(7, 7))
        != 0) {
        return false;
    }

    // Estimate extrinsic camera parameters using calibrating template
    if (mCameras[index]->SetExtrinsic(img) != 0) {
        return false;
    }
    return true;
}

bool CalibrationApp::GetContours(std::vector<std::shared_ptr<Texture>> videoTextures)
{
    for (uint i = 0U; i < mCameras.size(); i++) {
        if (SearchContours(i, videoTextures[i])) {
            float* contours = nullptr;
            int size = mCameras[i]->CreateContours(&contours);
            if (contours != nullptr) {
                mContoursMeshes[i]->LoadVertices((const float3*)contours, size / 3);
                free(contours);
            } else {
                LogError("Getting countours failed");
                return false;
            }
        }
    }
    return true;
}

bool CalibrationApp::GetGrids()
{
    int sum_num = 0;
    if (mCameras.size() > 0U) {
        int nopz = mSettings->gridPointsZCount;
        for (uint i = 0U; i < mCameras.size(); i++) { // Get number of points in z axis
            int tmp = mCameras[i]->GetBowlHeight(mSettings->bowlRadius * mCameras[i]->GetBaseRadius(),
                mSettings->gridStepX);
            nopz = MIN(nopz, tmp);
        }

        std::vector<std::vector<cv::Point3f>> seam;
        for (uint i = 0U; i < mGrids.size(); i++) {
            *mGrids[i] =
                CurvilinearGrid(mSettings->gridAngles, mSettings->gridStartAngle, nopz, mSettings->gridStepX);
            if (mGrids[i] == NULL) {
                continue;
            }
            // Calculate grid points and save grid to the file
            mGrids[i]->CreateGrid(mCameras[i], mSettings->bowlRadius * mCameras[i]->GetBaseRadius());

            // Getting grid meshes
            float* grid = nullptr;
            int size = mGrids[i]->GetGrid(&grid);
            if (grid != nullptr) {
                mGridMeshes[i]->LoadVertices((const float3*)grid, size / 3);
                free(grid);
            } else {
                LogError("Getting grids failed");
                return false;
            }
        }
    }

    return true;
}

void CalibrationApp::SaveGrids(void)
{
    // Grids
    std::vector<std::vector<cv::Point3f>> seams;
    std::vector<cv::Point3f> seam_points;
    for (uint i = 0U; i < mGrids.size(); i++) {
        mGrids[i]->SaveGrid(mCameras[i]);
        mGrids[i]->GetSeamPoints(seam_points);
        seams.push_back(seam_points);
    }

    // Masks
    Masks masks;
    masks.CreateMasks(mCameras, seams, mSettings->smoothAngle);
    if (masks.SplitGrids() == -1) {
        LogError("Texels/vertices grids have not been split");
    }

    // Exposure correction
    Compensator compensator(cv::Size(mSettings->displayWidth, mSettings->displayHeight));
    compensator.Feed(mCameras.size(), seams);
    AssetManager::MakeDirectory(GENERATED_DIR);
    AssetManager::MakeDirectory(COMPENSATOR_DIR);
    if (compensator.Save((const char*)COMPENSATOR_DIR) == -1) {
        LogError("Compensator grids have not been saved");
    }
}
