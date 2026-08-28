/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ImageCV.hpp"

#include "AssetManager.hpp"
#include "Log.hpp"

ImageCV::ImageCV(std::string source, int width, int height)
    : VideoStream(source, width, height)
{
    int buf_len = (int)(mHeight * mWidth * 3);

    std::string filePath = AssetManager::GetPath(std::string(mSource));
    mFrame = cv::imread(filePath.c_str(), cv::IMREAD_COLOR);
    if (mFrame.empty()) {
        LogError("%s file is not found", mSource.c_str());
        return;
    }
    cv::cvtColor(mFrame, mFrame, (int)cv::COLOR_BGR2RGB);
    Buffer buffer;
    buffer.start = (unsigned char*)(mFrame.data);
    buffer.offset = 0;
    buffer.width = mWidth;
    buffer.height = mHeight;
    buffer.format = VideoFormat::RGB;
    mTexture = std::make_shared<TextureBuffer>(buffer);
}

ImageCV::~ImageCV(void)
{
    Stop();
}

bool ImageCV::Start(void)
{
    return true;
}

void ImageCV::Stop(void) { }

std::shared_ptr<Texture> ImageCV::GetTexture()
{
    return mTexture;
}
