/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "WindowNative.hpp"

class Window
{
public:
    Window(std::shared_ptr<WindowNative> native);
    virtual ~Window(void);

    void Refresh(void);

    std::shared_ptr<Events> GetEvents();
    EGLDisplay GetDisplay();
    EGLContext GetContext();

protected:
    EGLDisplay mEglDisplay = nullptr;
    EGLConfig mEglConfig = nullptr;
    EGLSurface mEglSurface = nullptr;
    EGLContext mEglContext = nullptr;
    std::shared_ptr<WindowNative> mNative;
};
