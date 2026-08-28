/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Application.hpp"
#include "Log.hpp"

Application::Application(std::shared_ptr<Settings> settings, std::shared_ptr<View> view)
    : NotifiableOnEvent(view->GetEvents())
    , mSettings(settings)
    , mView(view)
{
}

void Application::RequestQuit()
{
    LogInfo("Quit requested");
    mQuit = true;
}

bool Application::QuitRequested()
{
    return mQuit;
}
