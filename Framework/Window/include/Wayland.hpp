/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Settings.hpp"
#include "WaylandSeat.hpp"
#include "WaylandShellXdg.hpp"
#include "WindowNative.hpp"

#include <cinttypes>
#include <memory>
#include <string>

struct wl_compositor;
struct wl_egl_window;
struct wl_display;
struct wl_surface;
struct wl_shm;
struct wl_registry;
struct wl_registry_listener;

class Wayland : public WindowNative
{
public:
    Wayland(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events);
    virtual ~Wayland();

    // Inherited from WindowNative
    void Refresh();
    EGLNativeDisplayType GetDisplay();
    EGLNativeWindowType GetWindow();

    void ResizeWindow(uint32_t width, uint32_t height);
    wl_compositor* GetCompositor();
    std::string GetAppId();
    bool IsFullscreen();
    bool IsSurfaceConfigured();

protected:
    wl_compositor* mCompositor;
    wl_egl_window* mWindow;
    wl_display* mDisplay;
    wl_surface* mSurface;
    wl_shm* mShm;
    bool mIsSurfaceConfigured;
    std::unique_ptr<WaylandShellXdg> mShell;
    std::unique_ptr<WaylandSeats> mSeats;
    std::string mAppId;
    uint32_t mWidth;
    uint32_t mHeight;
    bool mIsWindowResized;
    bool mIsFullscreen;

    void CreateDisplay(void);
    void CreateWindow(void);

    // Wayland callbacks
    static const wl_registry_listener mRegistryListener;
    static void RegistryGlobal(void* data, wl_registry* registry, uint32_t name, const char* interface,
        uint32_t version);
    static void RegistryGlobalRemove(void* data, wl_registry* registry, uint32_t name);
};
