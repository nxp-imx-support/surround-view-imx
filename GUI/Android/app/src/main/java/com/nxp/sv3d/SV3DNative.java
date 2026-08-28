/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

package com.nxp.sv3d;

import android.content.res.AssetManager;
import android.content.Context;
import android.view.MotionEvent;

public class SV3DNative
{
    public static native void Init(AssetManager assetManager, String filesDirPath);
    public static native void Update();
    public static native void SetApp(SV3DApp app);
    public static native void DeInit();
    public static native void ReloadXmlFile();
    public static native void OnTouchEvent(MotionEvent event);
    public static native boolean OnKeyUp(int keycode);
    public static native boolean OnKeyDown(int keycode);
    public static native void OnCameraPermissionGranted();
}