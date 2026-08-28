/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "TextureBuffer.hpp"
#include "VideoStream.hpp"

#include <memory>
#include <vector>

#include <opencv2/opencv.hpp>
#include <pthread.h>

#define BUFFER_NUM 4

class VideoCV : public VideoStream
{
public:
    VideoCV(std::string source, int width, int height);
    ~VideoCV(void);

    // Inherited from VideoStream
    virtual bool Start(void) override;
    virtual void Stop(void) override;
    virtual std::shared_ptr<Texture> GetTexture() override;

protected:
    cv::Mat mFrames[BUFFER_NUM];
    int mFrameNum;
    pthread_t mCaptureThread = 0;
    std::vector<std::shared_ptr<TextureBuffer>> mTextures;
    int mBufferIndex;

    static int mThreadId;
    static int mThreadIdCurrent;
    static int mFrameCurrent;
    static pthread_mutex_t mFrameMutex;

    static void* CaptureThread(void* data);
};
