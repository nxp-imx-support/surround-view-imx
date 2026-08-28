/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Settings.hpp"
#include "VideoStream.hpp"
#include "Window.hpp"

class VideoStreams
{
public:
    VideoStreams(std::shared_ptr<Settings> settings, std::shared_ptr<Window> window);
    virtual ~VideoStreams() = default;

    std::vector<std::shared_ptr<Texture>> GetTextures();

protected:
    std::vector<std::shared_ptr<VideoStream>> mVideoStreams;
};
