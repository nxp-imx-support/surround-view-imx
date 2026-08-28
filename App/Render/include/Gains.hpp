/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Compensator.hpp"

#include "opencv2/highgui/highgui.hpp"

#include <atomic>
#include <mutex>
#include <thread>

#define CHANELS_NUM 4
#define CAMERAS_NUM 4

class Gains
{
public:
    Gains(int width, int height);
    virtual ~Gains(void);

    cv::Mat& GetOverlapROI(int cameraId, int overlapId);
    float* GetGain(int cameraId);
    cv::Rect GetFlipROI(int cameraId);

    bool TryLock();
    void Unlock();
    void TriggerUpdate();

private:
    // Thread
    std::atomic<bool> mThreadRunning { false };
    std::thread mThread;
    std::mutex mGainMutex;
    std::mutex mTriggerMutex;

    cv::Mat mOverlapROI[CAMERAS_NUM][2];
    float mGain[CAMERAS_NUM][CHANELS_NUM];
    std::unique_ptr<Compensator> mCompensator;

    void Update();
};
