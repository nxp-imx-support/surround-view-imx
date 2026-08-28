/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Texture.hpp"
#include "VideoFormat.hpp"
#include <mutex>
#include <stdlib.h>

struct Buffer
{
    unsigned char* start = nullptr;
    size_t offset = 0;
    unsigned int bufferLength = 0;
    unsigned int width;
    unsigned int height;
    VideoFormat format;
};

class TextureBuffer : public Texture
{
public:
    TextureBuffer(Buffer buffer);
    virtual ~TextureBuffer();

    // Inherited from Texture
    virtual void Bind() override;
    virtual void OnUpdate() override;
    virtual cv::Mat GetData() override;

protected:
    Buffer mBuffer;
    bool mUpdateRequested = true;
    std::mutex mUpdateMutex;
};
