/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

package com.nxp.sv3d;

import java.util.HashMap;
import java.io.IOException;

import android.opengl.GLSurfaceView;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.EGLConfig;

import android.util.Log;
import android.content.res.AssetManager;

import android.view.Surface;
import android.graphics.SurfaceTexture;

public class SV3DRenderer implements GLSurfaceView.Renderer
{
    public static final String LOG_TAG = "SV3DRenderer";

    private SV3DActivity mActivity;

    private SV3DApp mPreviousApp;
    private SV3DApp mDisplayedApp;

    static private HashMap<String, SurfaceTexture> mSurfaceTextures = new HashMap<>();

    public SV3DRenderer(SV3DActivity activity)
    {
        mActivity = activity;
        mPreviousApp = SV3DApp.NONE;
        mDisplayedApp = SV3DApp.NONE;
    }

    public void onSurfaceCreated(GL10 unused, EGLConfig config)
    {
        SV3DNative.Init(mActivity.getContext().getAssets(), mActivity.getFilesDir().getPath());
    }

    public void onDrawFrame(GL10 unused)
    {
        for (SurfaceTexture surfaceTexture : mSurfaceTextures.values()) {
            surfaceTexture.updateTexImage();
        }
        SV3DNative.Update();
    }

    public void onSurfaceChanged(GL10 unused, int width, int height)
    {
        if (mDisplayedApp != mPreviousApp) {
            SV3DNative.SetApp(mDisplayedApp);
            mPreviousApp = mDisplayedApp;
        }
        else {
            //SV3DNative.Resize();
        }
    }

    public void SetApp(SV3DApp app)
    {
        mDisplayedApp = app;
    }

    public void ResetApp()
    {
        mPreviousApp = SV3DApp.NONE;
    }

    public static Surface getSurface(int textureId, int width, int height)
    {
        SurfaceTexture surfaceTexture = new SurfaceTexture(textureId);
        surfaceTexture.setDefaultBufferSize(width, height);
        mSurfaceTextures.put(String.valueOf(textureId), surfaceTexture);
        Surface surface = new Surface(surfaceTexture);
        return surface;
    }
}
