/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Mesh.hpp"
#include "Performances.hpp"
#include "Programs.hpp"
#include "Tensorflow.hpp"
#include "Texture.hpp"
#include "Time.hpp"
#include "View.hpp"

#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

using tflite::Tensorflow;

class ObjectDetection
{
public:
    ObjectDetection(uint32_t cameraCount, std::shared_ptr<View> view, uint32_t maxIPS, std::shared_ptr<Performances> performances);
    virtual ~ObjectDetection();

    virtual void SetTextures(std::vector<std::shared_ptr<Texture>> textures);
    void Update();

    std::vector<std::shared_ptr<Texture>> GetBoxesTextures();
    void RenderQuads(std::vector<std::shared_ptr<Texture>> textures, std::shared_ptr<View> view);

protected:
    uint32_t mCameraCount { 0 };
    std::shared_ptr<View> mView;
    Tensorflow mTensorflow;

    Mesh mCornerMesh;

    ProgramTexturedQuad mProgramQuad;
    ProgramTexturedQuadExt mProgramQuadExt;

    std::vector<std::shared_ptr<Texture>> mDefisheyeTextures;
    std::vector<GLuint> mDefisheyeFbo;

    std::vector<GLuint> mBoxesFbos;
    std::vector<std::shared_ptr<Texture>> mBoxesTextures;
    ProgramBoxQuad mBoxQuadProgram;

    uint32_t mWidth;
    uint32_t mHeight;

    uint32_t mFrameCount { 0 };

    std::vector<int> mCategories;
    float mThreshold;

    std::atomic<bool> mThreadRunning { false };
    std::thread mThread;

    using Element = uint8_t;
    using Buffer = Element*;
    using Pool = Buffer*;
    std::vector<Pool> mFreePools;
    std::mutex mFreeMutex;
    std::queue<Pool> mLockPools;
    std::mutex mLockMutex;

    std::vector<std::vector<DetectedObject>> mDetectedObjects;
    std::mutex mMutex;

    bool mNewDetection { false };
    uint32_t mMaxIPS = 0;
    double mMinDetectionTime = 0.0;
    Time mLastDetectionTime;

    std::shared_ptr<Performances> mPerformances;
};
