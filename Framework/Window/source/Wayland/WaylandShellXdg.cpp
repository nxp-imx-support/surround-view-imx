/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "WaylandShellXdg.hpp"
#include "Wayland.hpp"

#include "Log.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

#include "xdg-shell-protocol.h"
#include <wayland-client.h>
#include <wayland-egl.h>

// WmBase listeners
const xdg_wm_base_listener WaylandShellXdg::mWmBaseListener = { .ping = WaylandShellXdg::WmBasePing };

// Surface listeners
const xdg_surface_listener WaylandShellXdg::mSurfaceListener = { .configure = WaylandShellXdg::SurfaceConfigure };

// Toplevel listeners
const xdg_toplevel_listener WaylandShellXdg::mTopLevelListener = {
    .configure = WaylandShellXdg::TopLevelConfigure,
    .close = WaylandShellXdg::TopLevelClose,
};

WaylandShellXdg::WaylandShellXdg() { }

WaylandShellXdg::~WaylandShellXdg()
{
    if (mTopLevel != NULL) {
        xdg_toplevel_destroy(mTopLevel);
    }

    if (mSurface != NULL) {
        xdg_surface_destroy(mSurface);
    }
}

void WaylandShellXdg::BindRegistry(wl_registry* registry, uint32_t name, uint32_t version)
{
    LogDebug("Bind Wayland Shell XDG");
    mWmBase = static_cast<xdg_wm_base*>(
        wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, (uint32_t)1)));
    xdg_wm_base_add_listener(mWmBase, &mWmBaseListener, this);
}

bool WaylandShellXdg::CheckInterface(const char* interface)
{
    return (std::strcmp(interface, xdg_wm_base_interface.name) == 0);
}

wl_surface* WaylandShellXdg::CreateSurface(Wayland* wayland)
{
    wl_surface* waylandSurface = wl_compositor_create_surface(wayland->GetCompositor());

    if (waylandSurface == NULL) {
        LogError("wl_compositor_create_surface failed");
        return NULL;
    }

    mSurface = xdg_wm_base_get_xdg_surface(mWmBase, waylandSurface);

    if (mSurface == NULL) {
        LogError("xdg_wm_base_get_xdg_surface failed");
        wl_surface_destroy(waylandSurface);
        return NULL;
    }

    xdg_surface_add_listener(mSurface, &mSurfaceListener, this);

    mTopLevel = xdg_surface_get_toplevel(mSurface);
    xdg_toplevel_set_title(mTopLevel, "RRR");

    std::string appId = wayland->GetAppId();
    LogDebug("Setting app ID to: %s", appId.c_str());
    xdg_toplevel_set_app_id(mTopLevel, appId.c_str());

    xdg_toplevel_add_listener(mTopLevel, &mTopLevelListener, wayland);

    if (wayland->IsFullscreen() == true) {
        LogDebug("Set fullscreen");
        xdg_toplevel_set_fullscreen(mTopLevel, NULL);
    }

    wl_surface_set_user_data(waylandSurface, NULL);
    wl_surface_commit(waylandSurface);

    return waylandSurface;
}

void WaylandShellXdg::WmBasePing(void* data, xdg_wm_base* xdg_wm_base, uint32_t serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void WaylandShellXdg::SurfaceConfigure(void* data, xdg_surface* xdg_surface, uint32_t serial)
{
    WaylandShellXdg* shell = static_cast<WaylandShellXdg*>(data);
    xdg_surface_ack_configure(xdg_surface, serial);
    LogDebug("XDG surface configured");
    shell->mIsConfigured = true;
}

void WaylandShellXdg::TopLevelConfigure(void* data, xdg_toplevel* xdg_toplevel, int32_t width, int32_t height,
    wl_array* state)
{
    Wayland* wayland = static_cast<Wayland*>(data);
    if (width > 0 && height > 0) {
        wayland->ResizeWindow(width, height);
    }
}

void WaylandShellXdg::TopLevelClose(void* data, xdg_toplevel* xdg_toplevel) { }
