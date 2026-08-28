/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "VideoStreams.hpp"

#include "Log.hpp"

VideoStreams::VideoStreams(std::shared_ptr<Settings> settings, std::shared_ptr<Window> window)
{
    for (int i = 0; i < settings->camerasCount; i++) {
        mVideoStreams.push_back(VideoStream::Create(window, settings->cameras[i].source, settings->cameras[i].width,
            settings->cameras[i].height,
            StringToVideoFormat[settings->cameras[i].format]));
    }

    for (int i = 0; i < settings->camerasCount; i++) {
        if (mVideoStreams[i]->Start() == false) {
            LogError("Starting camera %d failed", i);
        }
    }
}

std::vector<std::shared_ptr<Texture>> VideoStreams::GetTextures()
{
    std::vector<std::shared_ptr<Texture>> textures;
    for (auto videoStream : mVideoStreams) {
        textures.push_back(videoStream->GetTexture());
    }
    return textures;
}
