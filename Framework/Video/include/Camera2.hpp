/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "VideoStream.hpp"

#include <mutex>
#include <string>

class _AndroidContext;

class Camera2 : public VideoStream
{
public:
    Camera2(std::string source, int width, int height, std::shared_ptr<Window> window);
    virtual ~Camera2();

    // Inherited from VideoStream
    virtual bool Start(void) override;
    virtual void Stop(void) override;
    virtual std::shared_ptr<Texture> GetTexture() override;

protected:
    std::unique_ptr<_AndroidContext> mContext;

    bool mHasNewFrame = false;

    std::shared_ptr<Texture> mTexture;
};
