/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <string.h>

#include "Events.hpp"
#include "WaylandSeat.hpp"

#define LOG_DEBUG 0
#include "Log.hpp"

#include <linux/input-event-codes.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Pointer
const wl_pointer_listener WaylandSeat::mPointerListener = {
    .enter = WaylandSeat::PointerEnter,
    .leave = WaylandSeat::PointerLeave,
    .motion = WaylandSeat::PointerMotion,
    .button = WaylandSeat::PointerButton,
    .axis = WaylandSeat::PointerAxis,
    .frame = WaylandSeat::PointerFrame,
    .axis_source = WaylandSeat::PointerAxisSource,
    .axis_stop = WaylandSeat::PointerAxisStop,
    .axis_discrete = WaylandSeat::PointerAxisDiscrete,
};

WaylandSeat::WaylandSeat(std::shared_ptr<Events> events, uint32_t width, uint32_t height)
    : mSeat(nullptr)
    , mEvents(events)
    , mPointer(nullptr)
    , mPointerEvent(nullptr)
    , mLastPosition { -1, -1 }
    , mTouch(nullptr)
    , mTouchEvent(nullptr)
    , mKeyboard(nullptr)
    , mXkbState(nullptr)
    , mXkbContext(nullptr)
    , mxkbKeymap(nullptr)
    , mWidth(width)
    , mHeight(height)
{
    mXkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
}

WaylandSeat::~WaylandSeat()
{
    if (mPointer != nullptr) {
        if (mVersion >= 3) {
            wl_pointer_release(mPointer);
        } else {
            wl_pointer_destroy(mPointer);
        }
    }
    if (mTouch != nullptr) {
        if (mVersion >= 3) {
            wl_touch_release(mTouch);
        } else {
            wl_touch_destroy(mTouch);
        }
    }
    if (mKeyboard != nullptr) {
        if (mVersion >= 3) {
            wl_keyboard_release(mKeyboard);
        } else {
            wl_keyboard_destroy(mKeyboard);
        }
    }
    wl_seat_release(mSeat);
}

void WaylandSeat::PointerEnter(void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface,
    wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Pointer enter (seat %s)", seat->mName.c_str());

    // cursor_set(wl_pointer, serial);
}

void WaylandSeat::PointerLeave(void* data, wl_pointer* wl_pointer, uint32_t serial, wl_surface* surface)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Pointer leave (seat %s)", seat->mName.c_str());
}

void WaylandSeat::PointerMotion(void* data, wl_pointer* wl_pointer, uint32_t time, wl_fixed_t surface_x,
    wl_fixed_t surface_y)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Pointer motion (seat %s) x=%d y=%d", seat->mName.c_str(), wl_fixed_to_int(surface_x),
        wl_fixed_to_int(surface_y));

    Event event;
    event.Type = EventType::MouseMove;
    event.KeyValue = Key::Mouse;
    event.MouseOffset.x = wl_fixed_to_int(surface_x) - seat->mLastPosition.x;
    event.MouseOffset.y = seat->mHeight - wl_fixed_to_int(surface_y) - seat->mLastPosition.y;
    seat->mLastPosition.x = wl_fixed_to_int(surface_x);
    seat->mLastPosition.y = seat->mHeight - wl_fixed_to_int(surface_y);
    event.MousePos = seat->mLastPosition;
    seat->mEvents->PostEvent(event);
}

void WaylandSeat::PointerButton(void* data, wl_pointer* wl_pointer, uint32_t serial, uint32_t time, uint32_t button,
    uint32_t state)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Pointer button (seat %s) button=%u state=%u serial=%u", seat->mName.c_str(), button, state, serial);

    if (seat->mLastPosition.x < 30 && seat->mLastPosition.x >= 0 && seat->mLastPosition.y > (seat->mHeight - 30) && seat->mLastPosition.y >= 0) {
        Event exit;
        exit.Type = EventType::KeyPress;
        exit.KeyValue = Key::Esc;
        seat->mEvents->PostEvent(exit);
    } else {
        Event event;
        event.Type = (state == WL_POINTER_BUTTON_STATE_PRESSED) ? EventType::MousePress : EventType::MouseRelease;
        event.KeyValue = Key::Mouse;
        event.MousePos = seat->mLastPosition;
        event.MouseOffset = Point { 0, 0 };
        seat->mEvents->PostEvent(event);
    }
}

void WaylandSeat::PointerAxis(void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
    auto seat = static_cast<WaylandSeat*>(data);
    Event event;
    event.Type = EventType::MouseScroll;
    event.KeyValue = Key::Mouse;
    event.MousePos = seat->mLastPosition;
    event.MouseOffset = Point { 0, 0 };
    event.ScrollValue = -wl_fixed_to_int(value);
    seat->mEvents->PostEvent(event);

    LogDebug("Pointer axis (seat %s) axis=%u value=%d", seat->mName.c_str(), axis, wl_fixed_to_int(value));
}

void WaylandSeat::PointerAxisSource(void* data, wl_pointer* wl_pointer, uint32_t axis_source)
{
    auto seat = static_cast<WaylandSeat*>(data);
}

void WaylandSeat::PointerAxisStop(void* data, wl_pointer* wl_pointer, uint32_t time, uint32_t axis)
{
    auto seat = static_cast<WaylandSeat*>(data);
}

void WaylandSeat::PointerAxisDiscrete(void* data, wl_pointer* wl_pointer, uint32_t axis, int32_t discrete)
{
    auto seat = static_cast<WaylandSeat*>(data);
}

void WaylandSeat::PointerFrame(void* data, wl_pointer* wl_pointer)
{
    auto seat = static_cast<WaylandSeat*>(data);
}

// Touch
const wl_touch_listener WaylandSeat::mTouchListener = {
    .down = WaylandSeat::TouchDown,
    .up = WaylandSeat::TouchUp,
    .motion = WaylandSeat::TouchMotion,
    .frame = WaylandSeat::TouchFrame,
};

void WaylandSeat::TouchDown(void* data, wl_touch* wl_touch, uint32_t serial, uint32_t time, wl_surface* surface,
    int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Touch Down (seat %s) id=%d x=%d y=%d", seat->mName.c_str(), id, wl_fixed_to_int(x), wl_fixed_to_int(y));

    Point point { wl_fixed_to_int(x), (int)seat->mHeight - wl_fixed_to_int(y) };
    seat->mTouchPoints.insert({ id, point });

    if (seat->mTouchPoints.size() == 2) {
        auto it = seat->mTouchPoints.begin();
        Point a = it->second;
        ++it;
        Point b = it->second;
        seat->mLastDistance = Point::Distance(a, b);
    } else {
        if (point.x < 30 && point.y > (seat->mHeight - 30)) {
            Event exit;
            exit.Type = EventType::KeyPress;
            exit.KeyValue = Key::Esc;
            seat->mEvents->PostEvent(exit);
        } else {
            Event event;
            event.Type = EventType::MousePress;
            event.KeyValue = Key::Mouse;
            event.MousePos = point;
            event.MouseOffset = Point { 0, 0 };
            seat->mEvents->PostEvent(event);
        }
    }
}

void WaylandSeat::TouchUp(void* data, wl_touch* wl_touch, uint32_t serial, uint32_t time, int32_t id)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Touch Up (seat %s) id=%d", seat->mName.c_str(), id);

    Event event;
    event.Type = EventType::MouseRelease;
    event.KeyValue = Key::Mouse;
    event.MousePos = seat->mTouchPoints[id];
    event.MouseOffset = Point { 0, 0 };
    seat->mEvents->PostEvent(event);

    seat->mTouchPoints.erase(id);
}

void WaylandSeat::TouchMotion(void* data, wl_touch* wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Touch Motion (seat %s) id=%d x=%d y=%d", seat->mName.c_str(), id, wl_fixed_to_int(x), wl_fixed_to_int(y));

    Point point { wl_fixed_to_int(x), (int)seat->mHeight - wl_fixed_to_int(y) };
    if (seat->mTouchPoints.size() == 1) {
        // Touch move event for single touch
        Event event;
        event.Type = EventType::MouseMove;
        event.KeyValue = Key::Mouse;
        event.MouseOffset = point - seat->mTouchPoints[id];
        event.MousePos = point;
        seat->mEvents->PostEvent(event);
    } else if (seat->mTouchPoints.size() == 2) {
        // Simulate scroll event for multi-touch pinch gesture
        Event event;
        event.Type = EventType::MouseScroll;
        event.KeyValue = Key::Mouse;
        event.MousePos = point;
        event.MouseOffset = Point { 0, 0 };

        auto it = seat->mTouchPoints.begin();
        Point a = it->second;
        ++it;
        Point b = it->second;
        float distance = Point::Distance(a, b);
        event.ScrollValue = (int)(0.5 * (distance - seat->mLastDistance));
        seat->mLastDistance = distance;

        seat->mEvents->PostEvent(event);
    } else {
        // Don't handle more than 2 touch points
    }
    seat->mTouchPoints[id].x = point.x;
    seat->mTouchPoints[id].y = point.y;
}

void WaylandSeat::TouchFrame(void* data, wl_touch* wl_touch)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Touch frame (seat %s)", seat->mName.c_str());
}

const struct wl_keyboard_listener WaylandSeat::mKeyboardListener = {
    .keymap = WaylandSeat::KeyboardKeymap,
    .enter = WaylandSeat::KeyboardEnter,
    .leave = WaylandSeat::KeyboardLeave,
    .key = WaylandSeat::KeyboardKey,
    .modifiers = WaylandSeat::KeyboardModifiers,
    .repeat_info = WaylandSeat::KeyboardRepeatInfo,
};

void WaylandSeat::KeyboardKeymap(void* data, wl_keyboard* wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Keyboard Enter (seat %s) wl_keyboard=%p format=%u fd=%d size=%u", seat->mName.c_str(), wl_keyboard, format, fd, size);

    // assert(format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1);

    char* map_shm = (char*)mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map_shm == MAP_FAILED) {
        LogError("mmap failed");
    }

    struct xkb_keymap* xkb_keymap =
        xkb_keymap_new_from_string(seat->mXkbContext, map_shm, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
    munmap(map_shm, size);
    close(fd);

    struct xkb_state* xkb_state = xkb_state_new(xkb_keymap);
    xkb_keymap_unref(seat->mxkbKeymap);
    xkb_state_unref(seat->mXkbState);
    seat->mxkbKeymap = xkb_keymap;
    seat->mXkbState = xkb_state;
}

void WaylandSeat::KeyboardEnter(void* data, wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface,
    wl_array* keys)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Keyboard Enter (seat %s)", seat->mName.c_str());
}

void WaylandSeat::KeyboardLeave(void* data, wl_keyboard* wl_keyboard, uint32_t serial, wl_surface* surface)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Keyboard Leave (seat %s)", seat->mName.c_str());
}

void WaylandSeat::KeyboardKey(void* data, wl_keyboard* wl_keyboard, uint32_t serial, uint32_t time, uint32_t key,
    uint32_t state)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Keyboard Key (seat %s)", seat->mName.c_str());

    Event event;
    event.Type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? EventType::KeyPress : EventType::KeyRelease;

    char buf[128];
    uint32_t keycode = key + 8;
    xkb_keysym_t sym = xkb_state_key_get_one_sym(seat->mXkbState, keycode);
    xkb_keysym_get_name(sym, buf, sizeof(buf));
#if LOG_DEBUG == 1
    xkb_state_key_get_utf8(seat->mXkbState, keycode, buf, sizeof(buf));
    LogDebug("utf8: '%s'", buf);
#endif
    xkb_keysym_t keyUpper = xkb_keysym_to_upper(sym);
    LogDebug("key %s: sym: %-12s (0x%X) (upper:0x%X), ", (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? "press" : "release",
        buf, sym, keyUpper);

    // Handle necessary keys only
    switch (keyUpper) {
    case 0x50:
        event.KeyValue = Key::P;
        break;
    case 0xFF52:
        event.KeyValue = Key::Up;
        break;
    case 0xFF54:
        event.KeyValue = Key::Down;
        break;
    case 0xFF51:
        event.KeyValue = Key::Left;
        break;
    case 0xFF53:
        event.KeyValue = Key::Right;
        break;
    case 0xFF1B:
        event.KeyValue = Key::Esc;
        break;
    case 0xFFBE:
        event.KeyValue = Key::F1;
        break;
    case 0xFFC2:
        event.KeyValue = Key::F5;
        break;
    default:
        event.KeyValue = Key::Unknown;
    }

    seat->mEvents->PostEvent(event);
}

void WaylandSeat::KeyboardModifiers(void* data, wl_keyboard* wl_keyboard, uint32_t serial, uint32_t mods_depressed,
    uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Keyboard Key (seat %s)", seat->mName.c_str());
    xkb_state_update_mask(seat->mXkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);
}

void WaylandSeat::KeyboardRepeatInfo(void* data, wl_keyboard* wl_keyboard, int32_t rate, int32_t delay)
{
    auto seat = static_cast<WaylandSeat*>(data);
    LogDebug("Keyboard Repeat Info (seat %s)", seat->mName.c_str());
}

const wl_seat_listener WaylandSeat::mSeatListener = {
    .capabilities = WaylandSeat::SeatCapabilities,
    .name = WaylandSeat::SeatName,
};

void WaylandSeat::SeatCapabilities(void* data, wl_seat* wl_seat, uint32_t capabilities)
{
    auto seat = static_cast<WaylandSeat*>(data);

    LogDebug("Capabilities: 0x%X (seat %s)", capabilities, seat->mName.c_str());

    // Pointer
    bool have_pointer = capabilities & WL_SEAT_CAPABILITY_POINTER;
    if (have_pointer && seat->mPointer == nullptr) {
        LogDebug("Pointer capability (seat %s)", seat->mName.c_str());
        seat->mPointer = wl_seat_get_pointer(seat->mSeat);
        wl_pointer_add_listener(seat->mPointer, &mPointerListener, seat);
    } else if (!have_pointer && seat->mPointer != nullptr) {
        LogDebug("NO Pointer capability (seat %s)", seat->mName.c_str());
        if (seat->mVersion >= 3) {
            wl_pointer_release(seat->mPointer);
        } else {
            wl_pointer_destroy(seat->mPointer);
        }
        seat->mPointer = nullptr;
    }

    // Touch
    bool have_touch = capabilities & WL_SEAT_CAPABILITY_TOUCH;
    if (have_touch && seat->mTouch == nullptr) {
        LogDebug("Touch capability (seat %s)", seat->mName.c_str());
        seat->mTouch = wl_seat_get_touch(seat->mSeat);
        wl_touch_add_listener(seat->mTouch, &mTouchListener, seat);
    } else if (!have_touch && seat->mTouch != nullptr) {
        LogDebug("NO Touch capability (seat %s)", seat->mName.c_str());
        if (seat->mVersion >= 3) {
            wl_touch_release(seat->mTouch);
        } else {
            wl_touch_destroy(seat->mTouch);
        }
        seat->mTouch = nullptr;
    }

    bool have_keyboard = capabilities & WL_SEAT_CAPABILITY_KEYBOARD;
    if (have_keyboard && seat->mKeyboard == nullptr) {
        LogDebug("Keyboard capability (seat %s)", seat->mName.c_str());
        seat->mKeyboard = wl_seat_get_keyboard(seat->mSeat);
        wl_keyboard_add_listener(seat->mKeyboard, &mKeyboardListener, seat);
    } else if (!have_keyboard && seat->mKeyboard != nullptr) {
        if (seat->mVersion >= 3) {
            wl_keyboard_release(seat->mKeyboard);
        } else {
            wl_keyboard_destroy(seat->mKeyboard);
        }
        seat->mKeyboard = nullptr;
    }
}

void WaylandSeat::SeatName(void* data, wl_seat* wl_seat, const char* name)
{
    auto seat = static_cast<WaylandSeat*>(data);
    seat->mName = name;
    LogDebug("Seat name: %s", seat->mName.c_str());
}

void WaylandSeat::BindRegistry(wl_registry* registry, uint32_t name, uint32_t version)
{
    LogDebug("Bind Seat: %u version=%u", name, version);
    mVersion = MIN(version, 2u);
    mSeat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, mVersion));
    wl_seat_add_listener(mSeat, &mSeatListener, this);
}

bool WaylandSeat::CheckInterface(const char* interface)
{
    return (strcmp(interface, wl_seat_interface.name) == 0);
}

WaylandSeats::WaylandSeats(std::shared_ptr<Events> events, uint32_t width, uint32_t height)
    : mEvents(events)
    , mWidth(width)
    , mHeight(height)
{
}

WaylandSeats::~WaylandSeats()
{
    mSeats.clear();
}

void WaylandSeats::BindRegistry(wl_registry* registry, uint32_t name, uint32_t version)
{
    auto seat = std::make_shared<WaylandSeat>(mEvents, mWidth, mHeight);
    seat->BindRegistry(registry, name, version);
    mSeats.push_back(seat);
}

bool WaylandSeats::CheckInterface(const char* interface)
{
    return WaylandSeat::CheckInterface(interface);
}
