/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Settings.hpp"
#include "Texture.hpp"
#include "View.hpp"

#include <cinttypes>

class Application : public NotifiableOnEvent
{
public:
    Application(std::shared_ptr<Settings> settings, std::shared_ptr<View> view);
    virtual ~Application() = default;

    virtual void OnEvent(const Event& inEvent) override { };

    virtual void Update(std::vector<std::shared_ptr<Texture>> videoTextures) = 0;
    void RequestQuit();
    bool QuitRequested();

protected:
    std::shared_ptr<Settings> mSettings;
    std::shared_ptr<View> mView;
    bool mQuit = false;
    uint32_t mCameraCount = 0;
};
