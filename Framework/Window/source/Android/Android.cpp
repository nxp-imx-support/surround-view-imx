/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Android.hpp"

#include "Log.hpp"

Android::Android(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events)
    : WindowNative(events)
{
    // Window is created by Android.
}

Android::~Android(void) { }

void Android::Refresh()
{
    // Buffer swapping is done in Java code.
}

EGLNativeDisplayType Android::GetDisplay()
{
    return static_cast<EGLNativeDisplayType>(nullptr);
}

EGLNativeWindowType Android::GetWindow()
{
    return static_cast<EGLNativeWindowType>(0);
}

void Android::OnTouchEvent(JNIEnv* jenv, jobject motionEvent)
{
    jclass motionEventClass = jenv->GetObjectClass(motionEvent);

    jmethodID getActionMaskedMethodId = jenv->GetMethodID(motionEventClass, "getActionMasked", "()I");
    int actionMasked = jenv->CallIntMethod(motionEvent, getActionMaskedMethodId);

    jmethodID getPointerCountMethodId = jenv->GetMethodID(motionEventClass, "getPointerCount", "()I");
    int pointerCount = jenv->CallIntMethod(motionEvent, getPointerCountMethodId);

    jmethodID getPointerIdMethodId = jenv->GetMethodID(motionEventClass, "getPointerId", "(I)I");
    jmethodID getXMethodId = jenv->GetMethodID(motionEventClass, "getX", "(I)F");
    jmethodID getYMethodId = jenv->GetMethodID(motionEventClass, "getY", "(I)F");

    // Get action constants
    jint actionMove = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "ACTION_MOVE", "I"));
    jint actionDown = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "ACTION_DOWN", "I"));
    jint actionUp = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "ACTION_UP", "I"));
    jint actionPointerDown = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "ACTION_POINTER_DOWN", "I"));
    jint actionPointerUp = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "ACTION_POINTER_UP", "I"));
    jint actionScroll = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "ACTION_SCROLL", "I"));

    LogInfo("Touch event - actionMasked: %d, pointerCount: %d", actionMasked, pointerCount);

    // Handle different action types
    if (actionMasked == actionMove) {
        // ACTION_MOVE updates ALL active pointers, so iterate through all of them
        for (int i = 0; i < pointerCount; i++) {
            int id = jenv->CallIntMethod(motionEvent, getPointerIdMethodId, i);
            float x = jenv->CallFloatMethod(motionEvent, getXMethodId, i);
            float y = jenv->CallFloatMethod(motionEvent, getYMethodId, i);
            mTouchPoints[id] = Point(static_cast<int>(x), static_cast<int>(y));
        }

        if (pointerCount == 1) {
            // Single touch move
            int id = jenv->CallIntMethod(motionEvent, getPointerIdMethodId, 0);
            float newX = jenv->CallFloatMethod(motionEvent, getXMethodId, 0);
            float newY = jenv->CallFloatMethod(motionEvent, getYMethodId, 0);
            Point point(static_cast<int>(newX), static_cast<int>(newY));

            Event event;
            event.Type = EventType::MouseMove;
            event.KeyValue = Key::Mouse;
            event.MouseOffset.x = newX - mMouseLastPosition.x;
            event.MouseOffset.y = -(newY - mMouseLastPosition.y);
            mMouseLastPosition = point;
            event.MousePos = point;
            mEvents->PostEvent(event);
        } else if (pointerCount == 2) {
            // Two-finger pinch/zoom gesture
            auto it = mTouchPoints.begin();
            Point a = it->second;
            ++it;
            Point b = it->second;
            float distance = Point::Distance(a, b);

            Event event;
            event.Type = EventType::MouseScroll;
            event.KeyValue = Key::Mouse;
            event.MousePos = b;
            event.MouseOffset = Point { 0, 0 };
            event.ScrollValue = (int)(0.5 * (distance - mLastDistance));
            mLastDistance = distance;
            mEvents->PostEvent(event);
        }
    } else if (actionMasked == actionDown || actionMasked == actionPointerDown) {
        // DOWN events
        jmethodID getActionIndexMethodId = jenv->GetMethodID(motionEventClass, "getActionIndex", "()I");
        int actionIndex = jenv->CallIntMethod(motionEvent, getActionIndexMethodId);

        int id = jenv->CallIntMethod(motionEvent, getPointerIdMethodId, actionIndex);
        float x = jenv->CallFloatMethod(motionEvent, getXMethodId, actionIndex);
        float y = jenv->CallFloatMethod(motionEvent, getYMethodId, actionIndex);
        Point point(static_cast<int>(x), static_cast<int>(y));
        mTouchPoints[id] = point;

        if (pointerCount == 2) {
            // Initialize distance for pinch gesture
            auto it = mTouchPoints.begin();
            Point a = it->second;
            ++it;
            Point b = it->second;
            mLastDistance = Point::Distance(a, b);
        } else if (pointerCount == 1) {
            // Single touch press
            Event event;
            event.Type = EventType::MousePress;
            event.KeyValue = Key::Mouse;
            event.MouseOffset = Point { 0, 0 };
            mMouseLastPosition = point;
            event.MousePos = point;
            mEvents->PostEvent(event);
        }
    } else if (actionMasked == actionUp || actionMasked == actionPointerUp) {
        // UP events
        jmethodID getActionIndexMethodId = jenv->GetMethodID(motionEventClass, "getActionIndex", "()I");
        int actionIndex = jenv->CallIntMethod(motionEvent, getActionIndexMethodId);

        int id = jenv->CallIntMethod(motionEvent, getPointerIdMethodId, actionIndex);

        if (mTouchPoints.find(id) != mTouchPoints.end()) {
            Event event;
            event.KeyValue = Key::Mouse;
            event.Type = EventType::MouseRelease;
            event.MousePos = mTouchPoints[id];
            event.MouseOffset = Point { 0, 0 };
            mEvents->PostEvent(event);
            mTouchPoints.erase(id);
        }
    } else if (actionMasked == actionScroll) {
        jint axisVscroll = jenv->GetStaticIntField(motionEventClass, jenv->GetStaticFieldID(motionEventClass, "AXIS_VSCROLL", "I"));
        jmethodID getAxisValueMethodId = jenv->GetMethodID(motionEventClass, "getAxisValue", "(I)F");

        Event event;
        event.KeyValue = Key::Mouse;
        event.Type = EventType::MouseScroll;
        event.ScrollValue = jenv->CallFloatMethod(motionEvent, getAxisValueMethodId, axisVscroll);
        event.MousePos = mMouseLastPosition;
        event.MouseOffset = Point { 0, 0 };
        mEvents->PostEvent(event);
    }
}

jboolean Android::OnKeyUp(JNIEnv* jenv, jint keyCode)
{
    Event event;
    event.Type = EventType::KeyRelease;
    event.KeyValue = MapKey(jenv, keyCode);
    mEvents->PostEvent(event);

    return true;
}

jboolean Android::OnKeyDown(JNIEnv* jenv, jint keyCode)
{
    Event event;
    event.Type = EventType::KeyPress;
    event.KeyValue = MapKey(jenv, keyCode);
    mEvents->PostEvent(event);

    return true;
}

Key Android::MapKey(JNIEnv* jenv, jint keyCode)
{
    jclass keyEventClass = jenv->FindClass("android/view/KeyEvent");

#define JavaKeyValue(keycode) \
    jenv->GetStaticIntField(keyEventClass, jenv->GetStaticFieldID(keyEventClass, #keycode, "I"))

    if (keyCode == JavaKeyValue(KEYCODE_P)) {
        return Key::P;
    } else if (keyCode == JavaKeyValue(KEYCODE_DPAD_UP)) {
        return Key::Up;
    } else if (keyCode == JavaKeyValue(KEYCODE_DPAD_DOWN)) {
        return Key::Down;
    } else if (keyCode == JavaKeyValue(KEYCODE_DPAD_LEFT)) {
        return Key::Left;
    } else if (keyCode == JavaKeyValue(KEYCODE_DPAD_RIGHT)) {
        return Key::Right;
    } else if (keyCode == JavaKeyValue(KEYCODE_ESCAPE)) {
        return Key::Esc;
    } else if (keyCode == JavaKeyValue(KEYCODE_F1)) {
        return Key::F1;
    } else if (keyCode == JavaKeyValue(KEYCODE_F5)) {
        return Key::F5;
    }
    return Key::Unknown;
}
