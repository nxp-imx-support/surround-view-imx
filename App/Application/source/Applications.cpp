/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Applications.hpp"
#include "CalibrationApp.hpp"
#include "CapturingApp.hpp"
#include "Log.hpp"
#include "RenderApp.hpp"

#include <signal.h>

volatile bool Applications::mQuitAll = false;

namespace {
static const char* FPS = "FPS";
}

Applications::Applications(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events, int argc, char** argv)
    : mSettings(settings)
{
    // ctrl + C
    if (signal(SIGINT, Applications::SigHandler) == SIG_ERR) {
        LogError("Error setting up signal handlers");
    }

    // kill command
    if (signal(SIGTERM, Applications::SigHandler) == SIG_ERR) {
        LogError("Error setting up signal handlers");
    }
    // Count number of applications to launch
    uint32_t appCount = 0;
    for (int i = 1; i < argc; ++i) {
        std::string appStr(argv[i]);
        if (appStr.compare("capture") == 0 || appStr.compare("calib") == 0 || appStr.compare("render") == 0) {
            ++appCount;
        }
    }
    appCount = (appCount == 0) ? 1 : appCount;

    // Grid
    uint32_t width = mSettings->displayWidth;
    uint32_t height = mSettings->displayHeight;
    uint32_t row = (uint32_t)(std::sqrt((double)appCount));
    uint32_t col = std::ceil(appCount / (float)row);
    std::vector<std::shared_ptr<View>> views;

    for (uint32_t j = 0; j < row; ++j) {
        for (uint32_t i = 0; i < col; ++i) {
            auto view = std::make_shared<View>((i * width) / col, ((row - j - 1) * height) / row, width / col, height / row, events);
            views.push_back(view);
        }
    }

    // Performances
    auto perfView = std::make_shared<View>(0, 0, mSettings->displayWidth, mSettings->displayHeight, events);
    mPerformances = std::make_shared<Performances>(mSettings, perfView);

    // Start applications
    uint32_t appId = 0;
    if (argc < 2) {
        // Set Render as the default application
        mApplications.push_back(std::make_unique<RenderApp>(mSettings, views[appId++], mPerformances));
    } else {
        for (int i = 1; i < argc; ++i) {
            std::string appStr(argv[i]);
            if (appStr.compare("capture") == 0) {
                int cameraId = 0;
                if (argc > i + 1) {
                    std::string valStr(argv[i + 1]);
                    std::size_t pos {};
                    try {
                        const int value { std::stoi(valStr, &pos) };
                        ++i;
                        cameraId = value - 1;
                        if ((cameraId < 0) || (cameraId >= mSettings->camerasCount)) {
                            LogError("Camera numbers must be in [1, %d]", mSettings->camerasCount);
                            cameraId = 0;
                        }
                    } catch (std::invalid_argument const& ex) {
                        cameraId = 0;
                    }
                }
                LogInfo("Camera %d will be opened", cameraId + 1);
                mApplications.push_back(std::make_unique<CapturingApp>(mSettings, views[appId++], cameraId));
            } else if (appStr.compare("calib") == 0) {
                mApplications.push_back(std::make_unique<CalibrationApp>(mSettings, views[appId++]));
            } else if (appStr.compare("render") == 0) {
                mApplications.push_back(std::make_unique<RenderApp>(mSettings, views[appId++], mPerformances));
            } else {
                LogError("Unknown application %s", appStr.c_str());
            }
        }
    }

    // Frame time and max FPS
    if (mSettings->maxFPS > 0) {
        mMinRenderTime = 1000000.0 / (mSettings->maxFPS);
        mLastRenderTime = Time::Get();
    }
}

void Applications::Update(std::vector<std::shared_ptr<Texture>> videoTextures)
{
    View::Clear();
    for (auto it = mApplications.begin(); it != mApplications.end(); ++it) {
        (*it)->Update(videoTextures);
        if ((*it)->QuitRequested()) {
            mApplications.erase(it);
            break;
        }
    }

    if (mApplications.size() == 0) {
        mQuitAll = true;
    }

    mPerformances->Update(FPS);
    mPerformances->Draw();
}

void Applications::Wait()
{
    // Wait to restrain frame rate to MaxFPS setting
    if (mSettings->maxFPS) {
        Time newTime = Time::Get();
        double delta_time = (newTime - mLastRenderTime).GetUs();
        if (delta_time <= mMinRenderTime) {
            int64_t us = static_cast<int64_t>(mMinRenderTime - delta_time);
            std::this_thread::sleep_for(std::chrono::microseconds(us));
        }
        mLastRenderTime = Time::Get();
    }
}

void Applications::SigHandler(int code)
{
    LogInfo("Caught signal %d, setting flag to quit", code);
    mQuitAll = true;
}

bool Applications::QuitRequested()
{
    return mQuitAll;
}
