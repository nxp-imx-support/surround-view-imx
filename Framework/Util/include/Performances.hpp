/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "FontRenderer.hpp"
#include "Time.hpp"

#include <map>
#include <memory>

class Settings;
class View;

class Performance
{
public:
    Performance() = default;
    virtual ~Performance() = default;

    void Update();
    double GetFPS() const;

private:
    std::shared_ptr<View> mView;

    bool mIsStarted = false;
    unsigned int mCounter = 0U;
    unsigned int mUpdateCount = 1U;
    Time mPreviousTime;
    double mFPS = 0.0;

    std::unique_ptr<FontRenderer> mFontRenderer;
};

class Performances
{
public:
    Performances(std::shared_ptr<Settings> settings, std::shared_ptr<View> view);
    virtual ~Performances() = default;

    void Update(std::string id);
    double GetFPS(std::string id) const;
    void Draw();

private:
    std::shared_ptr<View> mView;

    std::unordered_map<std::string, std::unique_ptr<Performance>> mPerformances;
    std::unique_ptr<FontRenderer> mFontRenderer;
};
