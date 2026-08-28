/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ObjectDetection.hpp"

#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"

namespace {
static const char* IPS = "IPS";
}

ObjectDetection::ObjectDetection(uint32_t cameraCount, std::shared_ptr<View> view, uint32_t maxIPS, std::shared_ptr<Performances> performances)
    : mCameraCount(cameraCount)
    , mView(view)
    , mThreshold(0.65f)
    , mThread()
    , mMaxIPS(maxIPS)
    , mPerformances(performances)
{
    mWidth = mView->GetWidth();
    mHeight = mView->GetHeight();

    // Use 2 pools of input buffers
    mFreeMutex.lock();
    mFreePools.push_back(new Buffer[mCameraCount]);
    mFreePools.push_back(new Buffer[mCameraCount]);
    mFreeMutex.unlock();

    for (int i = 0; i < mCameraCount; i++) {
        // Defisheye textures
        std::shared_ptr<Texture> texture = std::make_shared<Texture>(mWidth, mHeight);
        texture->SetEmpty(Format::RGB8);
        mDefisheyeTextures.push_back(texture);

        // FBOs
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture->GetTarget(), texture->GetId(), 0);
        mDefisheyeFbo.push_back(framebuffer);

        // Mesh
        mCornerMesh.Load(QUAD_INV_Y);

        // Boxes textures
        // We should use GL_LUMINANCE textures with smaller size (half size) for better performance
        std::shared_ptr<Texture> boxesTexture = std::make_shared<Texture>(mWidth, mHeight);
        boxesTexture->SetEmpty(Format::RGBA8);
        mBoxesTextures.push_back(boxesTexture);

        // FBOs
        GLuint boxesFramebuffer;
        glGenFramebuffers(1, &boxesFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, boxesFramebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, boxesTexture->GetTarget(), boxesTexture->GetId(), 0);
        mBoxesFbos.push_back(boxesFramebuffer);

        // Create 1 buffer per camera per pool
        mFreeMutex.lock();
        for (auto pool : mFreePools) {
            pool[i] = new Element[mWidth * mHeight * 3];
        }
        mFreeMutex.unlock();

        mDetectedObjects.push_back({});
    }

    mCategories.push_back(0); // person
    mCategories.push_back(1); // bicycle
    mCategories.push_back(2); // car
    mCategories.push_back(3); // motorcycle
    mCategories.push_back(5); // bus
    mCategories.push_back(6); // train
    mCategories.push_back(7); // truck

    mCategories.push_back(16); // cat
    mCategories.push_back(17); // dog

    // Max Inference per second
    if (mMaxIPS) {
        mLastDetectionTime = Time::Get();
        mMinDetectionTime = 1000.0 / ((double)mMaxIPS / mCameraCount);
    }

    mThreadRunning.store(true, std::memory_order_release);
    mThread = std::thread(&ObjectDetection::Update, this);
}

ObjectDetection::~ObjectDetection()
{
    mThreadRunning.store(false, std::memory_order_release);
    if (mThread.joinable()) {
        mThread.join();
    }

    mFreeMutex.lock();
    for (Pool pool : mFreePools) {
        for (int camera = 0; camera < mCameraCount; ++camera) {
            delete[] pool[camera];
        }
        delete[] pool;
    }
    mFreeMutex.unlock();
    mLockMutex.lock();
    for (; !mLockPools.empty(); mLockPools.pop()) {
        Pool pool = mLockPools.front();
        for (int camera = 0; camera < mCameraCount; ++camera) {
            delete[] pool[camera];
        }
        delete[] pool;
    }
    mLockMutex.unlock();
}

void ObjectDetection::SetTextures(std::vector<std::shared_ptr<Texture>> textures)
{
    // Called from Render thread
    mFreeMutex.lock();
    if (mFreePools.empty() == false) {
        Pool pool = mFreePools.back();
        mFreePools.pop_back();
        mFreeMutex.unlock();

        LogDebug("SetTextures on %p", pool);
        mView->ViewportFull();

        for (int camera = 0; camera < mCameraCount; ++camera) {
            if (std::shared_ptr<Texture> texture = textures[camera]; texture != nullptr) {
                glDisable(GL_DEPTH_TEST);
                glDisable(GL_BLEND);
                mView->BindFramebuffer(mDefisheyeFbo[camera]);
                ProgramTexturedQuad& program = texture->IsExternalOES() ? mProgramQuadExt : mProgramQuad;
                program.Use();
                program.SetGain(1.0f);
                program.SetTexture(texture);
                mCornerMesh.Render(RenderPrimitive::Triangle);

                glReadPixels(0, 0, mWidth, mHeight, GL_RGB, GL_UNSIGNED_BYTE, pool[camera]);
            }
        }
        mLockMutex.lock();
        mLockPools.push(pool);
        mLockMutex.unlock();
    } else {
        mFreeMutex.unlock();
    }
}

std::vector<std::shared_ptr<Texture>> ObjectDetection::GetBoxesTextures()
{
    // Called from Render thread
    std::lock_guard<std::mutex> guard(mMutex);
    if (mNewDetection) {
        for (int camera = 0; camera < mCameraCount; ++camera) {
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendEquation(GL_MAX);
            mView->BindFramebuffer(mBoxesFbos[camera]);
            mView->Clear(0.0f, 0.0f, 0.0f, 0.0f);
            mBoxQuadProgram.Use();
            for (auto object : mDetectedObjects[camera]) {
                mBoxQuadProgram.SetGain(object.Box);
                mView->RenderQuad();
            }
        }
        glBlendEquation(GL_FUNC_ADD);
        mNewDetection = false;
    }
    return mBoxesTextures;
}

void ObjectDetection::RenderQuads(std::vector<std::shared_ptr<Texture>> textures, std::shared_ptr<View> view)
{
    GetBoxesTextures();

    // For debug purpose
    // Call this instead of Render() in RenderApp
    view->BindFramebuffer(0);
    view->Clear();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    for (int i = 0; i < mCameraCount; ++i) {
        view->ViewportCorner(i);
        if (std::shared_ptr<Texture> texture = textures[i]; texture != nullptr) {
            ProgramTexturedQuad& program = texture->IsExternalOES() ? mProgramQuadExt : mProgramQuad;
            program.Use();
            program.SetGain(1.0f);
            program.SetTexture(texture);
            glDisable(GL_BLEND);
            view->RenderQuad();
            glEnable(GL_BLEND);
            mProgramQuad.Use();
            mProgramQuad.SetGain(1.0f, 0.0f, 0.0f, 0.0f);
            mProgramQuad.SetTexture(mBoxesTextures[i]);
            view->RenderQuad();
        }
    }
}

void ObjectDetection::Update()
{
    while (mThreadRunning.load(std::memory_order_acquire)) {
        mLockMutex.lock();
        if (mLockPools.empty() == false) {
            Pool pool = mLockPools.front();
            LogDebug("Update on %p", pool);
            mLockPools.pop();
            mLockMutex.unlock();

            for (int camera = 0; camera < mCameraCount; ++camera) {
                std::vector<DetectedObject> detectedObjects =
                    mTensorflow.RunInference(pool[camera], mWidth, mHeight, 3, mThreshold, mCategories);
                std::lock_guard<std::mutex> guard(mMutex);
                mDetectedObjects[camera].clear();
                for (auto object : detectedObjects) {
                    mDetectedObjects[camera].push_back(object);
                }
                mPerformances->Update("IPS");
            }
            mFreeMutex.lock();
            mFreePools.push_back(pool);
            mFreeMutex.unlock();
            mNewDetection = true;
        } else {
            mLockMutex.unlock();
        }
        // Wait
        if (mMaxIPS) {
            Time newTime = Time::Get();
            double delta_time = (newTime - mLastDetectionTime).GetMs();
            if (delta_time < mMinDetectionTime) {
                auto ms = static_cast<int64_t>(mMinDetectionTime - delta_time);
                std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            }
            mLastDetectionTime = Time::Get();
        }
    }
}
