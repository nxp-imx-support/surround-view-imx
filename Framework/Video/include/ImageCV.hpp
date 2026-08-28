/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "TextureBuffer.hpp"
#include "VideoStream.hpp"

#include <memory>
#include <opencv2/opencv.hpp>

class ImageCV : public VideoStream
{
public:
    ImageCV(std::string source, int width, int height);
    ~ImageCV(void);

    // Inherited from VideoStream
    virtual bool Start(void) override;
    virtual void Stop(void) override;
    virtual std::shared_ptr<Texture> GetTexture() override;

protected:
    cv::Mat mFrame;
    std::shared_ptr<TextureBuffer> mTexture;
};
