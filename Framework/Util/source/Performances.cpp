/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Performances.hpp"

#include "FilesName.hpp"
#include "Log.hpp"
#include "Settings.hpp"
#include "View.hpp"

#include <cinttypes>

static const uint32_t FastUpdateCount = 10U;
static const uint32_t SlowUpdateCount = 100U;

void Performance::Update()
{
    if (mIsStarted == false) {
        mPreviousTime = Time::Get();
        mIsStarted = true;
        mUpdateCount = FastUpdateCount;
        mCounter = 0U;
    } else {
        if (mCounter == mUpdateCount) {
            Time currentTime = Time::Get();
            Time deltaTime = currentTime - mPreviousTime;
            mPreviousTime = currentTime;
            mFPS = mCounter / (deltaTime.GetMs() / 1000.0);
            mUpdateCount = SlowUpdateCount;
            mCounter = 0U;
        }
    }
    ++mCounter;
}

double Performance::GetFPS() const
{
    return mFPS;
}

Performances::Performances(std::shared_ptr<Settings> settings, std::shared_ptr<View> view)
    : mView(view)
{
    mFontRenderer = std::make_unique<FontRenderer>(settings->displayWidth, settings->displayHeight, FONT_PATH_FILE);
}

void Performances::Update(std::string id)
{
    if (mPerformances.find(id) == mPerformances.end()) {
        mPerformances[id] = std::make_unique<Performance>();
    }
    mPerformances[id]->Update();
}

double Performances::GetFPS(std::string id) const
{
    return mPerformances.at(id)->GetFPS();
}

void Performances::Draw()
{
    mView->ViewportFull();
    float top = 2.0f;
    for (const auto& performance : mPerformances) {
        std::string fpsText = performance.first + std::string(":") + std::to_string((int)(performance.second->GetFPS() + 0.5));
        mFontRenderer->RenderText(fpsText.c_str(), top, 2.0f);
        top += 3.0f;
    }
}
