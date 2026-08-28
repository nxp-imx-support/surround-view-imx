/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cinttypes>

struct wl_registry;
struct wl_surface;
struct wl_array;
struct xdg_wm_base;
struct xdg_wm_base_listener;
struct xdg_surface;
struct xdg_surface_listener;
struct xdg_toplevel;
struct xdg_toplevel_listener;

class Wayland;

class WaylandShellXdg
{
public:
    WaylandShellXdg();
    virtual ~WaylandShellXdg();

    void BindRegistry(wl_registry* registry, uint32_t name, uint32_t version);
    bool CheckInterface(const char* interface);
    wl_surface* CreateSurface(Wayland* wayland);
    virtual bool IsConfigured() { return mIsConfigured; }

protected:
    bool mIsConfigured { false };

    xdg_wm_base* mWmBase;
    xdg_surface* mSurface;
    xdg_toplevel* mTopLevel;

    // XDG WmBase Callbacks
    static const xdg_wm_base_listener mWmBaseListener;
    static void WmBasePing(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial);

    // XDG Surface Callbacks
    static const xdg_surface_listener mSurfaceListener;
    static void SurfaceConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial);

    // XDG TopLevel Callbacks
    static const xdg_toplevel_listener mTopLevelListener;
    static void TopLevelConfigure(void* data, xdg_toplevel* xdg_toplevel, int32_t width, int32_t height,
        wl_array* state);
    static void TopLevelClose(void* data, xdg_toplevel* xdg_toplevel);
};
