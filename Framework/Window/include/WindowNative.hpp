/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Events.hpp"
#include <EGL/egl.h>

class WindowNative
{
public:
    WindowNative(std::shared_ptr<Events> events)
        : mEvents(events) { };
    virtual ~WindowNative(void) { };

    std::shared_ptr<Events> GetEvents() { return mEvents; };

    virtual void Refresh() = 0;
    virtual EGLNativeDisplayType GetDisplay() = 0;
    virtual EGLNativeWindowType GetWindow() = 0;

protected:
    std::shared_ptr<Events> mEvents;
};
