/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RenderApp.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Texture.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>

#include <thread>

#define CAR_ORIENTATION_X 90.0f
#define CAR_ORIENTATION_Y 270.0f
#define CAM_LIMIT_RY_MIN -1.57f
#define CAM_LIMIT_RY_MAX 0.0f
#define CAM_LIMIT_ZOOM_MIN -11.5f
#define CAM_LIMIT_ZOOM_MAX -2.5f
#define CAM_ZOOM_SPEED 0.05f
#define CAM_MOVE_SPEED 0.2f

RenderApp::RenderApp(std::shared_ptr<Settings> settings, std::shared_ptr<View> view, std::shared_ptr<Performances> performances)
    : Application(settings, view)
{
    mWidth = mView->GetWidth();
    mHeight = mView->GetHeight();

    // Exposure correction
    mGain = std::make_unique<Gains>(mWidth, mHeight);
    mEcFrameCnt = 0;

    // Transform matrices
    InitMatrices();

    // Load vertices arrays
    for (int i = 0; i < mCameraCount; i++) {
        // Overlap region
        auto overlapMesh = std::make_shared<Mesh>();
        overlapMesh->Load(VERTICES_DIR ARRAY_PREFIX + std::to_string(i + 1) + "1");
        mOverlapMeshes.push_back(overlapMesh);

        // Non Overlap region
        auto nonOverlapMesh = std::make_shared<Mesh>();
        nonOverlapMesh->Load(VERTICES_DIR ARRAY_PREFIX + std::to_string(i + 1) + "2");
        mNonOverlapMeshes.push_back(nonOverlapMesh);

        // Exposure correction
        auto exposureCorrectionMesh = std::make_shared<Mesh>();
        exposureCorrectionMesh->Load(COMPENSATOR_DIR ARRAY_PREFIX + std::to_string(i + 1));
        mExposureCorrectionMeshes.push_back(exposureCorrectionMesh);

        // Mask texture
        std::string mask_name = AssetManager::GetPath(MASKS_DIR MASK_PREFIX + std::to_string(i) + ".jpg");
        cv::Mat mask = cv::imread(mask_name, cv::IMREAD_GRAYSCALE);
        std::shared_ptr<Texture> maskTexture = std::make_shared<Texture>(mask.cols, mask.rows);
        maskTexture->SetData(Format::GREY8, mask.data);
        mMaskTextures.push_back(maskTexture);
    }

    // Exposure correction
    InitEcTextures();

#ifdef USE_OBJDET
    if (mSettings->objDetEnable) {
        mObjectDetection = std::make_shared<ObjectDetection>(mCameraCount, std::make_shared<View>(0, 0, 300, 300, mView->GetEvents()), mSettings->maxIPS, performances);
    }
#endif
    LogDebug("Render initialisation done");
}

RenderApp::~RenderApp()
{
    DeInitEcTextures();
}

void RenderApp::Update(std::vector<std::shared_ptr<Texture>> videoTextures)
{
#ifdef USE_OBJDET
    if (mSettings->objDetEnable) {
        mObjectDetection->SetTextures(videoTextures);
    }
#endif
#if DEBUG_OBJECT_DETECTION == 1
#ifdef USE_OBJDET
    // Display grid of camera feeds with detection overlays
    if (mSettings->objDetEnable) {
        mObjectDetection->RenderQuads(videoTextures, mView);
    }
#endif
#else
    Render(videoTextures);
#endif
}

void RenderApp::OnEvent(const Event& inEvent)
{
    // Handle events
    switch (inEvent.Type) {
    // key pressing
    case EventType::KeyPress:
        switch (inEvent.KeyValue) {
        case Key::Esc:
            RequestQuit();
            break;
        case Key::Right:
            px += 0.5f;
            break;
        case Key::Left:
            px -= 0.5f;
            break;
        case Key::Up:
            py += 0.5f;
            break;
        case Key::Down:
            py -= 0.5f;
            break;
        case Key::F1:
            rx = 0.0f;
            ry = 0.0f;
            px = 0.0f;
            py = 0.0f;
            pz = -5.0f;
            break;
        default:
            break;
        }
        break;
    // Mouse moving
    case EventType::MouseMove:
        if (mEvents->IsPressed(Key::Mouse)) {
            rx += glm::radians(inEvent.MouseOffset.x * CAM_MOVE_SPEED);
            ry -= glm::radians(inEvent.MouseOffset.y * CAM_MOVE_SPEED);
            // prevent the camera to flip the model upside down and look under the model
            ry = glm::clamp(ry, CAM_LIMIT_RY_MIN, CAM_LIMIT_RY_MAX);
        }
        break;
    // Mouse scrolling
    case EventType::MouseScroll:
        pz += CAM_ZOOM_SPEED * inEvent.ScrollValue;
        pz = glm::clamp(pz, CAM_LIMIT_ZOOM_MIN, CAM_LIMIT_ZOOM_MAX); // prevent the camera from zooming too close
        break;
    default:
        break;
    }
}

void RenderApp::InitEcTextures(void)
{
    // Exposure correction
    glGenFramebuffers(1, &mEcFbo);
    glGenRenderbuffers(1, &mEcRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, mEcRbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, mWidth, mHeight);
    glBindFramebuffer(GL_FRAMEBUFFER, mEcFbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, mEcRbo);
}

void RenderApp::InitMatrices()
{
    mCameraCount = mSettings->camerasCount;

    mCarProjection = glm::perspective(45.0f, (float)mWidth / (float)mHeight, 0.1f, 100.0f);
    glm::mat4 scaleMatrix =
        glm::scale(glm::vec3(mSettings->modelScale[0], mSettings->modelScale[1], mSettings->modelScale[2]));
    mCarRotationMatrix = glm::rotate(glm::radians(CAR_ORIENTATION_X), glm::vec3(1, 0, 0));
    mCarRotationMatrix = mCarRotationMatrix * glm::rotate(glm::radians(CAR_ORIENTATION_Y), glm::vec3(0, 1, 0));
    mCarModelMatrix = mCarRotationMatrix * scaleMatrix;
}

void RenderApp::Render(std::vector<std::shared_ptr<Texture>> textures)
{
#ifdef USE_OBJDET
    std::vector<std::shared_ptr<Texture>> boxes;
    if (mSettings->objDetEnable) {
        boxes = mObjectDetection->GetBoxesTextures();
    }
#endif
    mView->BindFramebuffer(0);
    mView->ViewportFull();

    double fpsValue = 0.0;

    // Calculate ModelViewProjection matrix
    glm::mat4 translateMat = glm::translate(glm::vec3(px, py, pz));
    glm::mat4 rotateMat = glm::rotate(ry, glm::vec3(1, 0, 0)) * glm::rotate(rx, glm::vec3(0, 0, 1));
    glm::mat4 mv = translateMat * rotateMat;
    glm::mat4 mvp = mCarProjection * mv;

    // Render camera frames
    // Render overlap regions of camera frame with blending
    int halfCamNum = mCameraCount / 2;
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    for (int i = 0; i < mCameraCount; ++i) {
        // Reorder indices {0 1 2 3} to {0 2 1 3}
        // To render non-adjacent cameras without blending and blend other cameras over
        int camera = 2 * (i % halfCamNum) + (i / halfCamNum);
        if (std::shared_ptr<Texture> texture = textures[camera]; texture != nullptr) {
            ProgramGainMask& overlapProgram = texture->IsExternalOES() ? mOverlapExtProgram : mOverlapProgram;
            overlapProgram.Use();
            overlapProgram.SetTransform(glm::value_ptr(mvp));
            overlapProgram.SetMask(mMaskTextures[camera]);
            overlapProgram.SetGain(mGain->GetGain(camera));
            overlapProgram.SetTexture(texture);

            if (i == halfCamNum) {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }

            mOverlapMeshes[camera]->Render();
        }
    }

    // Render non-overlap region of camera frame without blending
    glDisable(GL_BLEND);
    for (int camera = 0; camera < mCameraCount; ++camera) {
        if (std::shared_ptr<Texture> texture = textures[camera]; texture != nullptr) {
            ProgramGain& nonOverlapProgram = texture->IsExternalOES() ? mNonOverlapExtProgram : mNonOverlapProgram;
            nonOverlapProgram.Use();
            nonOverlapProgram.SetTransform(glm::value_ptr(mvp));
            nonOverlapProgram.SetGain(mGain->GetGain(camera));
            nonOverlapProgram.SetTexture(texture);

            mNonOverlapMeshes[camera]->Render();
        }
    }

#ifdef USE_OBJDET
    if (mSettings->objDetEnable) {
        // Render detected boxes
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        for (int camera = 0; camera < mCameraCount; ++camera) {
            if (std::shared_ptr<Texture> box = boxes[camera]; box != nullptr) {
                mNonOverlapProgram.Use();
                mNonOverlapProgram.SetTransform(glm::value_ptr(mvp));
                mNonOverlapProgram.SetGain(10.0f, 0.0f, 0.0f, 1.0f);
                mNonOverlapProgram.SetTexture(box);
                mNonOverlapMeshes[camera]->Render();
            }
        }

        for (int camera = 0; camera < mCameraCount; ++camera) {
            if (std::shared_ptr<Texture> box = boxes[camera]; box != nullptr) {
                mOverlapProgram.Use();
                mOverlapProgram.SetTransform(glm::value_ptr(mvp));
                mOverlapProgram.SetGain(10.0f, 0.0f, 0.0f, 1.0f);
                mOverlapProgram.SetTexture(box);
                mOverlapProgram.SetMask(mMaskTextures[camera]);
                mOverlapMeshes[camera]->Render();
            }
        }
    }
#endif

    // Render car model
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    mCarModelProgram.Use();
    mCarModelProgram.SetTransform(glm::value_ptr(mvp * mCarModelMatrix));
    mCarModelProgram.SetModelView(glm::value_ptr(mv));
    mCarModelProgram.SetMormalMat(glm::value_ptr(glm::mat3(rotateMat * mCarRotationMatrix)));

    mCarModel.Draw(&mCarModelProgram);

    if (mSettings->ecRefreshRate) {
        if (mEcFrameCnt++ >= mSettings->ecRefreshRate) {
            // Exposure correction
            if (mGain->TryLock()) {
                mView->BindFramebuffer(mEcFbo);
                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);

                // Render camera overlap regions
                for (int camera = 0; camera < mCameraCount; camera++) {
                    if (std::shared_ptr<Texture> texture = textures[camera]; texture != nullptr) {
                        ProgramTexturedQuad& exposureCorrectionProgram =
                            texture->IsExternalOES() ? mExposureCorrectionExtProgram : mExposureCorrectionProgram;
                        exposureCorrectionProgram.Use();
                        exposureCorrectionProgram.SetGain(1.0f);
                        exposureCorrectionProgram.SetTexture(texture);

                        // Render camera overlap regions
                        mView->Clear(0.0f, 0.0f, 0.0f, 0.0f);
                        mView->Viewport(0.0f, 0.0f, mView->GetWidth(), mView->GetHeight());
                        mExposureCorrectionMeshes[camera]->Render();

                        // Read buffer
                        cv::Rect ROI = mGain->GetFlipROI(camera);
                        glReadPixels(ROI.x, ROI.y, ROI.width, ROI.height, GL_RGBA, GL_UNSIGNED_BYTE,
                            mGain->GetOverlapROI(camera, 0).data);

                        ROI = mGain->GetFlipROI((camera + 1) % mCameraCount);
                        glReadPixels(ROI.x, ROI.y, ROI.width, ROI.height, GL_RGBA, GL_UNSIGNED_BYTE,
                            mGain->GetOverlapROI(camera, 1).data);
                    }
                }
                mView->BindFramebuffer(0);

                // Release gain mutex
                mGain->Unlock();
                mGain->TriggerUpdate();
                mEcFrameCnt = 0;
            }
        }
    }
}

void RenderApp::DeInitEcTextures(void)
{
    glDeleteRenderbuffers(1, &mEcRbo);
    glDeleteFramebuffers(1, &mEcFbo);
}
