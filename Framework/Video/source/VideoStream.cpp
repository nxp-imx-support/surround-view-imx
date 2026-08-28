/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "VideoStream.hpp"

#include "CameraV4L2.hpp"
#ifdef ANDROID
#include "Camera2.hpp"
#else
#include "GstPlayer.hpp"
#endif
#include "ImageCV.hpp"
#include "Log.hpp"
#include "VideoCV.hpp"
#include "Window.hpp"

VideoStream::VideoStream(std::string source, int width, int height, VideoFormat videoFormat)
    : mSource(source)
    , mWidth(width)
    , mHeight(height)
    , mVideoFormat(videoFormat)
{
}

VideoStream::~VideoStream(void) { }

std::shared_ptr<VideoStream> VideoStream::Create(std::shared_ptr<Window> window, std::string source, int width, int height, VideoFormat videoFormat)
{
    std::size_t colon = source.find(":");
    std::string type = source.substr(0, colon);
    std::string path = source.substr(colon + 1);

    if (type == "v4l2") {
        return std::make_shared<CameraV4L2>(path, width, height, videoFormat);
    } else if (type == "img") {
        return std::make_shared<ImageCV>(path, width, height);
    } else if (type == "vid") {
        return std::make_shared<VideoCV>(path, width, height);
#ifdef ANDROID
    } else if (type == "cam2") {
        return std::make_shared<Camera2>(path, width, height, window);
#else
    } else if (type == "gst") {
        return std::make_shared<GstPlayer>(path, width, height, window);
#endif
    }
    return nullptr;
}

int VideoStream::GetWidth()
{
    return mWidth;
}

int VideoStream::GetHeight()
{
    return mHeight;
}
