/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "VideoFormat.hpp"

#include <memory>
#include <opencv2/opencv.hpp>
#include <string>

class Window;
class Texture;

class VideoStream
{
public:
    VideoStream(std::string source, int width, int height, VideoFormat videoFormat = VideoFormat::RGB);
    virtual ~VideoStream(void);

    static std::shared_ptr<VideoStream> Create(std::shared_ptr<Window> window,
        std::string source, int width, int height,
        VideoFormat videoFormat = VideoFormat::RGB);

    virtual bool Start(void) = 0;
    virtual void Stop(void) = 0;
    virtual std::shared_ptr<Texture> GetTexture() = 0;

    int GetWidth();
    int GetHeight();

protected:
    int mWidth;
    int mHeight;
    VideoFormat mVideoFormat;
    bool mIsRunning = false;
    std::string mSource;
};
