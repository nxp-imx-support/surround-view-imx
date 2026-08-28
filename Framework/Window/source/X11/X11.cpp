/*
 * Copyright 2017, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "X11.hpp"

#include "Log.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

// Save X11 Event names before undefining the macros
namespace x11 {
constexpr int XKeyPress = KeyPress;
constexpr int XKeyRelease = KeyRelease;
constexpr int XButtonPress = ButtonPress;
constexpr int XButtonRelease = ButtonRelease;
constexpr int XMotionNotify = MotionNotify;
}

// Undefine X11 macros that conflict with EventType enum
#undef KeyPress
#undef KeyRelease
#undef ButtonPress
#undef ButtonRelease
#undef MotionNotify

X11Window::X11Window(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events)
    : WindowNative(events)
    , mWidth(settings->displayWidth)
    , mHeight(settings->displayHeight)
{
    mDisplay = XOpenDisplay(NULL);

    if (mDisplay == NULL) {
        LogError("Failed to open X11 display");
    }

    int screen = DefaultScreen(mDisplay);
    Window rootwindow = RootWindow(mDisplay, screen);
    mWindow = XCreateSimpleWindow(mDisplay, rootwindow, 0, 0, mWidth, mHeight, 0, 0, BlackPixel(mDisplay, screen));
    XMapWindow(mDisplay, mWindow);
    XStoreName(mDisplay, mWindow, "SV3D");
    XSelectInput(mDisplay, mWindow, KeyPressMask | PointerMotionMask | ButtonPressMask);
    XFlush(mDisplay);
}

X11Window::~X11Window(void)
{
    XCloseDisplay(mDisplay);
}

void X11Window::Refresh()
{
    for (int i = 0; i < XPending(mDisplay); i++) {
        bool unsupported = false;
        XEvent xevent;
        Event event;
        XNextEvent(mDisplay, &xevent);

        switch (xevent.type) {
        case x11::XKeyPress:
        case x11::XKeyRelease: {
            event.Type = (xevent.type == x11::XKeyPress) ? EventType::KeyPress : EventType::KeyRelease;
            KeySym keysym = XLookupKeysym(&xevent.xkey, 0);
            if (keysym == (KeySym)XK_Escape) {
                event.KeyValue = Key::Esc;
            } else if (keysym == (KeySym)XK_Right) {
                event.KeyValue = Key::Right;
            } else if (keysym == (KeySym)XK_Left) {
                event.KeyValue = Key::Left;
            } else if (keysym == (KeySym)XK_Up) {
                event.KeyValue = Key::Up;
            } else if (keysym == (KeySym)XK_Down) {
                event.KeyValue = Key::Down;
            } else if (keysym == (KeySym)XK_F1) {
                event.KeyValue = Key::F1;
            } else if (keysym == (KeySym)XK_F5) {
                event.KeyValue = Key::F5;
            } else if (keysym == (KeySym)XK_p) {
                event.KeyValue = Key::P;
            } else {
                event.KeyValue = Key::Esc;
            }
            break;
        }
        case x11::XMotionNotify:
            event.Type = EventType::MouseMove;
            event.KeyValue = Key::Mouse;
            event.MouseOffset.x = xevent.xmotion.x - mMouseLastPosition.x;
            event.MouseOffset.y = mHeight - xevent.xmotion.y - mMouseLastPosition.y;
            mMouseLastPosition.x = xevent.xmotion.x;
            mMouseLastPosition.y = mHeight - xevent.xmotion.y;
            event.MousePos = mMouseLastPosition;
            break;
        case x11::XButtonPress:
        case x11::XButtonRelease:
            event.KeyValue = Key::Mouse;
            event.MousePos = mMouseLastPosition;
            event.MouseOffset = Point { 0, 0 };
            if (xevent.xbutton.button == (uint)Button1) {
                // Left click
                event.Type = (xevent.type == x11::XButtonPress) ? EventType::MousePress : EventType::MouseRelease;
                if (mMouseLastPosition.x < 30 && mMouseLastPosition.x >= 0 && mMouseLastPosition.y > (mHeight - 30) && mMouseLastPosition.y >= 0) {
                    // Hack exit button
                    Event exit;
                    exit.Type = EventType::KeyPress;
                    exit.KeyValue = Key::Esc;
                    mEvents->PostEvent(exit);
                    unsupported = true;
                }
            } else if (xevent.xbutton.button == (uint)Button4) {
                // Scroll up
                event.Type = EventType::MouseScroll;
                event.ScrollValue = 1;
            } else if (xevent.xbutton.button == (uint)Button5) {
                // Scroll down
                event.Type = EventType::MouseScroll;
                event.ScrollValue = -1;
            }
            break;
        default:
            unsupported = true;
            break;
        }
        if (!unsupported) {
            mEvents->PostEvent(event);
        }
    }
}

EGLNativeDisplayType X11Window::GetDisplay()
{
    return static_cast<EGLNativeDisplayType>(mDisplay);
}

EGLNativeWindowType X11Window::GetWindow()
{
    return static_cast<EGLNativeWindowType>(mWindow);
}
