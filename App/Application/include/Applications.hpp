/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Application.hpp"
#include "Performances.hpp"
#include "Time.hpp"

#include <cinttypes>

class Applications
{
public:
    Applications(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events, int argc, char** argv);
    virtual ~Applications() = default;

    void Update(std::vector<std::shared_ptr<Texture>> videoTextures);
    void Wait();
    bool QuitRequested();

protected:
    std::shared_ptr<Settings> mSettings;
    std::vector<std::unique_ptr<Application>> mApplications;

    std::shared_ptr<Performances> mPerformances;
    double mMinRenderTime = 0.0;
    Time mLastRenderTime;

    static volatile bool mQuitAll;
    static void SigHandler(int sig_code);
};
