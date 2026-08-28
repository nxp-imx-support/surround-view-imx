/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Events.hpp"

NotifiableOnEvent::NotifiableOnEvent(std::shared_ptr<Events> events)
{
    mEvents = events;
    mEvents->RegisterListener(this);
}

NotifiableOnEvent::~NotifiableOnEvent()
{
    mEvents->UnRegisterListener(this);
}

void Events::PostEvent(Event event)
{
    if (event.Type == EventType::KeyPress || event.Type == EventType::MousePress) {
        mPressedKeyList.push_back(event.KeyValue);
    } else if (event.Type == EventType::KeyRelease || event.Type == EventType::MouseRelease) {
        mPressedKeyList.remove(event.KeyValue);
    }

    for (NotifiableOnEvent* notifiable : mNotifiableOnEventList) {
        notifiable->OnEvent(event);
    }
}

void Events::RegisterListener(NotifiableOnEvent* listener)
{
    bool found = false;
    for (auto l : mNotifiableOnEventList) {
        if (listener == l) {
            found = true;
        }
    }
    if (!found) {
        mNotifiableOnEventList.push_back(listener);
    }
}

void Events::UnRegisterListener(NotifiableOnEvent* listener)
{
    for (auto i = mNotifiableOnEventList.begin(); i != mNotifiableOnEventList.end(); ++i) {
        if (listener == *i) {
            mNotifiableOnEventList.erase(i);
            return;
        }
    }
}

bool Events::IsPressed(Key key)
{
    for (Key k : mPressedKeyList) {
        if (key == k) {
            return true;
        }
    }
    return false;
}
