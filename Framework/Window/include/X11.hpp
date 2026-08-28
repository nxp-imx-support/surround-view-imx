/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

// Workaround egl pkg-config adding -DWL_EGL_PLATFORM
#undef WL_EGL_PLATFORM

#include "Settings.hpp"
#include "WindowNative.hpp"

#include <cinttypes>

struct _XDisplay;
typedef struct _XDisplay Display;
typedef unsigned long XWindow;

class X11Window : public WindowNative
{
public:
    X11Window(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events);
    virtual ~X11Window();

    // Inherited from WindowNative
    void Refresh();
    EGLNativeDisplayType GetDisplay();
    EGLNativeWindowType GetWindow();

protected:
    Display* mDisplay;
    XWindow mWindow;
    uint32_t mWidth;
    uint32_t mHeight;
    Point mMouseLastPosition;
};
