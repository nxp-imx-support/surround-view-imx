/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "TextureBuffer.hpp"
#include "VideoStream.hpp"

#include <memory>
#include <vector>

#define BUFFER_NUM 4

class CameraV4L2 : public VideoStream
{
public:
    CameraV4L2(std::string source, int width, int height, VideoFormat videoFormat);
    ~CameraV4L2(void);

    // Inherited from VideoStream
    virtual bool Start(void) override;
    virtual void Stop(void) override;
    virtual std::shared_ptr<Texture> GetTexture() override;

protected:
    const int mMemoryType;
    int mFd = -1;
    Buffer mBuffers[BUFFER_NUM];
    v4l2_buffer mV4L2Buffers[BUFFER_NUM] = {};
    std::vector<std::shared_ptr<TextureBuffer>> mTextures;
    pthread_t mCaptureThread = 0;
    int mBufferIndex;
    // Calling VIDIOC_STREAMOFF on one capture device freezes all other devices.
    // So application cannot be quitted properly
    static bool mIsRunning_Workaround;

    void StartStreaming(void);
    void StartThread(void);
    static void* CaptureThread(void* data);
};
