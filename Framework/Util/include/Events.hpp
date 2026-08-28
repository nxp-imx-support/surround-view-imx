/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <list>
#include <memory>
#include <vector>

#include "Point.hpp"

// Type of input event
enum class EventType
{
    MousePress,
    MouseRelease,
    MouseMove,
    MouseScroll,
    KeyPress,
    KeyRelease
};

enum class Key
{
    Esc,
    Up,
    Down,
    Right,
    Left,
    F1,
    F5,
    P,
    Mouse,
    Unknown
};

struct Event
{
    EventType Type;
    int ScrollValue;
    Key KeyValue;
    Point MousePos;
    Point MouseOffset;
};

class Events;
class NotifiableOnEvent
{
public:
    NotifiableOnEvent(std::shared_ptr<Events> events);
    virtual ~NotifiableOnEvent();

    virtual void OnEvent(const Event& event) = 0;

protected:
    std::shared_ptr<Events> mEvents;
};

class Events
{
public:
    void PostEvent(Event event);
    void RegisterListener(NotifiableOnEvent* listener);
    void UnRegisterListener(NotifiableOnEvent* listener);
    bool IsPressed(Key key);

protected:
    std::vector<NotifiableOnEvent*> mNotifiableOnEventList;
    std::list<Key> mPressedKeyList;
};
