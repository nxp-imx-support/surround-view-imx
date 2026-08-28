/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Wayland.hpp"
#include "Log.hpp"
#include "WaylandShellXdg.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

#include <wayland-client.h>
#include <wayland-egl.h>

const wl_registry_listener Wayland::mRegistryListener = { .global = Wayland::RegistryGlobal,
    .global_remove = Wayland::RegistryGlobalRemove };

Wayland::Wayland(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events)
    : WindowNative(events)
{
    LogInfo("Windowing system: Wayland");

    // Default values
    mCompositor = NULL;
    mWindow = NULL;
    mDisplay = NULL;
    mSurface = NULL;
    mShm = NULL;
    mIsSurfaceConfigured = false;
    mWidth = settings->displayWidth;
    mHeight = settings->displayHeight;
    mShell = std::make_unique<WaylandShellXdg>();
    mSeats = std::make_unique<WaylandSeats>(events, mWidth, mHeight);
    mAppId = "SV3D";
    mIsWindowResized = false;
    mIsFullscreen = settings->fullscreen;

    CreateDisplay();
    CreateWindow();
}

Wayland::~Wayland()
{
    LogDebug("Release Wayland");
    mSeats.reset();
    mShell.reset();
    wl_surface_destroy(mSurface);
    wl_shm_destroy(mShm);
    wl_compositor_destroy(mCompositor);
    wl_display_disconnect(mDisplay);
}

void Wayland::Refresh()
{
    if (wl_display_dispatch_pending(mDisplay) < 0) {
        LogError("wl_display_dispatch_pending failed");
    }
}

EGLNativeDisplayType Wayland::GetDisplay()
{
    return static_cast<EGLNativeDisplayType>(mDisplay);
}

EGLNativeWindowType Wayland::GetWindow()
{
    return static_cast<EGLNativeWindowType>(mWindow);
}

void Wayland::CreateDisplay(void)
{
    wl_registry* registry;

    mDisplay = wl_display_connect(NULL);
    if (mDisplay == NULL) {
        LogError("wl_display_connect failed");
    }

    registry = wl_display_get_registry(mDisplay);
    wl_registry_add_listener(registry, &mRegistryListener, (void*)this);
    wl_display_roundtrip(mDisplay);
    wl_registry_destroy(registry);
}

void Wayland::CreateWindow(void)
{
    mSurface = mShell->CreateSurface(this);

    mWindow = wl_egl_window_create(mSurface, mWidth, mHeight);

    if (mWindow == NULL) {
        LogError("wl_egl_window_create failed");
    }
}

void Wayland::ResizeWindow(uint32_t width, uint32_t height)
{
    if (width > 0 && height > 0 && (width != mWidth || height != mHeight)) {
        LogDebug("Window size %ux%u", width, height);
        mWidth = width;
        mHeight = height;
        // Resize egl window if already created
        if (mWindow != NULL) {
            wl_egl_window_resize(mWindow, mWidth, mHeight, 0, 0);
        }
        mIsWindowResized = true;
    }
}

wl_compositor* Wayland::GetCompositor()
{
    return mCompositor;
}

std::string Wayland::GetAppId()
{
    return mAppId;
}

bool Wayland::IsFullscreen()
{
    return mIsFullscreen;
}

bool Wayland::IsSurfaceConfigured()
{
    if (mShell != nullptr) {
        return mShell->IsConfigured();
    }
    return false;
}

void Wayland::RegistryGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    Wayland* wayland = static_cast<Wayland*>(data);

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        LogDebug("Bind Wayland Compositor");
        wayland->mCompositor = static_cast<wl_compositor*>(
            wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, (uint32_t)4)));
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        LogDebug("Bind Wayland Shm");
        wayland->mShm =
            static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, std::min(version, (uint32_t)1)));
    } else if (wayland->mShell->CheckInterface(interface)) {
        wayland->mShell->BindRegistry(registry, name, version);
    } else if (wayland->mSeats->CheckInterface(interface)) {
        wayland->mSeats->BindRegistry(registry, name, version);
    }
}

void Wayland::RegistryGlobalRemove(void* data, wl_registry* registry, uint32_t name) { }
