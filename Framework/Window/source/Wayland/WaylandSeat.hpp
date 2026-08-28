/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once

#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>
#include <wayland-util.h>
#include <xkbcommon/xkbcommon.h>

#include "Events.hpp"
#include "Point.hpp"

struct wl_registry;
struct wl_surface;

struct wl_seat;
struct wl_seat_listener;

struct wl_pointer;
struct wl_pointer_listener;
struct PointerEvent;

struct wl_touch;
struct wl_touch_listener;
struct TouchEvent;

struct wl_keyboard;
struct wl_keyboard_listener;
struct xkb_state;
struct xkb_context;
struct xkb_keymap;

class Wayland;

class WaylandSeat
{
public:
    WaylandSeat(std::shared_ptr<Events> events, uint32_t width, uint32_t height);
    virtual ~WaylandSeat();

    void BindRegistry(wl_registry* registry, uint32_t name, uint32_t version);
    static bool CheckInterface(const char* interface);

protected:
    std::string mName;
    wl_seat* mSeat;
    uint32_t mVersion;
    std::shared_ptr<Events> mEvents;
    uint32_t mWidth;
    uint32_t mHeight;

    wl_pointer* mPointer;
    PointerEvent* mPointerEvent;
    Point mLastPosition;
    float mLastDistance;

    wl_touch* mTouch;
    TouchEvent* mTouchEvent;
    std::map<int32_t, Point> mTouchPoints;

    wl_keyboard* mKeyboard;
    xkb_state* mXkbState;
    xkb_context* mXkbContext;
    xkb_keymap* mxkbKeymap;

    // Listeners
    // Pointer
    static const wl_pointer_listener mPointerListener;
    static void PointerEnter(void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface,
        wl_fixed_t surface_x, wl_fixed_t surface_y);
    static void PointerLeave(void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface);
    static void PointerMotion(void* data, wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x,
        wl_fixed_t surface_y);
    static void PointerButton(void* data, wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button,
        uint32_t state);
    static void PointerAxis(void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
    static void PointerAxisSource(void* data, wl_pointer* wl_pointer, uint32_t axis_source);
    static void PointerAxisStop(void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis);
    static void PointerAxisDiscrete(void* data, wl_pointer* wl_pointer, uint32_t axis, int32_t discrete);
    static void PointerFrame(void* data, wl_pointer* wl_pointer);

    // Touch
    static const wl_touch_listener mTouchListener;
    static void TouchDown(void* data, wl_touch* wl_touch, uint32_t serial, uint32_t time, wl_surface* surface,
        int32_t id, wl_fixed_t x, wl_fixed_t y);
    static void TouchUp(void* data, wl_touch* wl_touch, uint32_t serial, uint32_t time, int32_t id);
    static void TouchMotion(void* data, wl_touch* wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y);
    static void TouchFrame(void* data, wl_touch* wl_touch);

    // Keyboard
    static const wl_keyboard_listener mKeyboardListener;
    static void KeyboardKeymap(void* data, wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size);
    static void KeyboardEnter(void* data, wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface,
        wl_array* keys);
    static void KeyboardLeave(void* data, wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface);
    static void KeyboardKey(void* data, wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key,
        uint32_t state);
    static void KeyboardModifiers(void* data, wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed,
        uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
    static void KeyboardRepeatInfo(void* data, wl_keyboard* wl_keyboard, int32_t rate, int32_t delay);

    // Seat
    static const wl_seat_listener mSeatListener;
    static void SeatCapabilities(void* data, wl_seat* wl_seat, uint32_t capabilities);
    static void SeatName(void* data, wl_seat* wl_seat, const char* name);
};

class WaylandSeats
{
public:
    WaylandSeats(std::shared_ptr<Events> events, uint32_t width, uint32_t height);
    virtual ~WaylandSeats();

    void BindRegistry(wl_registry* registry, uint32_t name, uint32_t version);
    bool CheckInterface(const char* interface);

protected:
    std::vector<std::shared_ptr<WaylandSeat>> mSeats;
    std::shared_ptr<Events> mEvents;
    uint32_t mWidth;
    uint32_t mHeight;
};
