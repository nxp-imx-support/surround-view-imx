/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Window.hpp"

#include "Log.hpp"

Window::Window(std::shared_ptr<WindowNative> native)
{
    EGLint error = EGL_SUCCESS;
    mNative = native;

    EGLint numconfigs;
    const EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SAMPLES, 4,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 8,
        EGL_NONE
    };

    mEglDisplay = eglGetDisplay(mNative->GetDisplay());
    if (eglInitialize(mEglDisplay, NULL, NULL) != EGL_TRUE) {
        LogError("eglInitialize failed with error 0x%X", eglGetError());
    }

    if (eglBindAPI(EGL_OPENGL_ES_API) != EGL_TRUE) {
        LogError("eglBindAPI failed with error 0x%X", eglGetError());
    }

    if (eglChooseConfig(mEglDisplay, configAttribs, &mEglConfig, 1, &numconfigs) != EGL_TRUE) {
        LogError("eglChooseConfig failed with error 0x%X", eglGetError());
    }

    mEglSurface = eglCreateWindowSurface(mEglDisplay, mEglConfig, mNative->GetWindow(), NULL);
    if (EGLint error = eglGetError(); error != EGL_SUCCESS) {
        LogError("eglCreateWindowSurface failed with error 0x%X", error);
    }

    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    mEglContext = eglCreateContext(mEglDisplay, mEglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (EGLint error = eglGetError(); error != EGL_SUCCESS) {
        LogError("eglCreateContext failed with error 0x%X", error);
    }

    if (eglMakeCurrent(mEglDisplay, mEglSurface, mEglSurface, mEglContext) != EGL_TRUE) {
        LogError("eglMakeCurrent failed with error 0x%X", eglGetError());
    }
    eglSwapInterval(mEglDisplay, 1);
}

Window::~Window(void)
{
    if (eglMakeCurrent(mEglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE) {
        LogError("eglMakeCurrent failed with error 0x%X", eglGetError());
    }
    if (eglTerminate(mEglDisplay) != EGL_TRUE) {
        LogError("eglTerminate failed with error 0x%X", eglGetError());
    }
}

void Window::Refresh(void)
{
    mNative->Refresh();
    if (eglSwapBuffers(mEglDisplay, mEglSurface) != EGL_TRUE) {
        LogError("eglSwapBuffers failed with error 0x%X", eglGetError());
    }
}

std::shared_ptr<Events> Window::GetEvents()
{
    return mNative->GetEvents();
}

EGLDisplay Window::GetDisplay()
{
    return mEglDisplay;
}

EGLContext Window::GetContext()
{
    return mEglContext;
}
