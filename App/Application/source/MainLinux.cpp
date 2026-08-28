/*
 * Copyright 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Applications.hpp"
#include "Events.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "VideoStreams.hpp"
#include "View.hpp"
#if defined SV3D_WAYLAND
#include "Wayland.hpp"
#elif defined SV3D_X11
#include "X11.hpp"
#endif

#include "Window.hpp"

int main(int argc, char** argv)
{
    // Read xml parameters
    auto settings = std::make_shared<Settings>();
    if (settings->ReadXML(SETTINGS_PATH_FILE) == -1) {
        exit(-1);
    }

    std::shared_ptr<Events> events = std::make_shared<Events>();
#if defined SV3D_WAYLAND
    std::shared_ptr<WindowNative> windowNative = std::make_shared<Wayland>(settings, events);
#elif defined SV3D_X11
    std::shared_ptr<WindowNative> windowNative = std::make_shared<X11Window>(settings, events);
#else
#error "Unknown Windowing System"
#endif
    std::shared_ptr<Window> window = std::make_shared<Window>(windowNative);
    VideoStreams videoStreams(settings, window);
    Applications applications(settings, events, argc, argv);

    // Main loop
    while (applications.QuitRequested() == false) {
        applications.Update(videoStreams.GetTextures());
        window->Refresh();
        applications.Wait();
    }

    LogInfo("Quitting");
    return 0;
}
