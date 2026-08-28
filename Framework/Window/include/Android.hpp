/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Point.hpp"
#include "Settings.hpp"
#include "WindowNative.hpp"

#include <map>
#include <memory>

#include <android/asset_manager.h>
#include <jni.h>

class Android : public WindowNative
{
public:
    Android(std::shared_ptr<Settings> settings, std::shared_ptr<Events> events);
    virtual ~Android();

    // Inherited from WindowNative
    void Refresh();
    EGLNativeDisplayType GetDisplay();
    EGLNativeWindowType GetWindow();

    void OnTouchEvent(JNIEnv* jenv, jobject motionEvent);
    jboolean OnKeyUp(JNIEnv* jenv, jint keyCode);
    jboolean OnKeyDown(JNIEnv* jenv, jint keyCode);

protected:
    Point mMouseLastPosition;
    std::map<int32_t, Point> mTouchPoints;
    float mLastDistance;

    Key MapKey(JNIEnv* jenv, jint keyCode);
};
