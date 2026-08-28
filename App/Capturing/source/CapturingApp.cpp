/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CapturingApp.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Texture.hpp"

CapturingApp::CapturingApp(std::shared_ptr<Settings> settings, std::shared_ptr<View> view, int cameraNum)
    : Application(settings, view)
    , mCurrentCamera(0)
    , mSingleCameraNum(cameraNum)
{
    // Initialize all cameras
    mCameraCount = mSettings->camerasCount;

    for (int i = 0; i < mCameraCount; ++i) {
        // Camera indices for frame capture
        CameraIndices camInd;
        camInd.CurrentFrame = 0;
        camInd.ParamIndex = i;
        mCameraIndices.push_back(camInd);
    }

    if (mSingleCameraNum < 0) {
        mCurrentCamera = 0;
    } else {
        mCurrentCamera = mSingleCameraNum;
    }

    // Create directory for outputs
    AssetManager::MakeDirectory(GENERATED_DIR);
    AssetManager::MakeDirectory(CHESSBOARDS_DIR);
    for (int i = 0; i < mCameraCount; ++i) {
        AssetManager::MakeDirectory(CHESSBOARDS_DIR);
    }
}

CapturingApp::~CapturingApp()
{
    mCameraIndices.clear();
}

void CapturingApp::Update(std::vector<std::shared_ptr<Texture>> videoTextures)
{
    if (mCaptureRequested) {
        std::string camI = std::to_string(mCameraIndices[mCurrentCamera].ParamIndex + 1);
        std::string frameI = std::to_string(mCameraIndices[mCurrentCamera].CurrentFrame);
        std::string frameFile = AssetManager::GetPath(
            CHESSBOARDS_DIR CHESSBOARD_PREFIX + camI + "/" + FRAME_PREFIX + camI + "_" + frameI + ".jpg");
        if (cv::imwrite(frameFile.c_str(), videoTextures[mCurrentCamera]->GetData()) == false) {
            LogError("Failed to save the image %s", frameFile.c_str());
        } else {
            LogInfo("Saved image at %s", frameFile.c_str());
            ++mCameraIndices[mCurrentCamera].CurrentFrame;
        }
        mCaptureRequested = false;
    }

    mView->BindFramebuffer(0);
    mView->ViewportFull();

    if (auto texture = videoTextures[mCurrentCamera]; texture != nullptr) {
        ProgramTexturedQuad& program = texture->IsExternalOES() ? mProgramExt : mProgram;
        program.Use();
        program.SetGain(1.0f);
        program.SetTexture(texture);
        mView->RenderQuad();
    }
}

void CapturingApp::OnEvent(const Event& inEvent)
{
    switch (inEvent.Type) {
    case EventType::KeyPress:
        switch (inEvent.KeyValue) {
        case Key::Esc:
            LogInfo("The exit key was pressed");
            RequestQuit();
            break;
        case Key::P: {
            mCaptureRequested = true;
            break;
        }
        case Key::Right:
            if (mCurrentCamera < 3) {
                ++mCurrentCamera;
            }
            break;
        case Key::Left:
            if (mCurrentCamera > 0) {
                --mCurrentCamera;
            }
            break;
        default:
            break;
        }
    default:
        break;
    }
}
