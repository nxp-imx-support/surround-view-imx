/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

package com.nxp.sv3d;

import android.util.Log;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;

import android.view.MotionEvent;
import android.view.KeyEvent;

class SV3DConfigChooser implements GLSurfaceView.EGLConfigChooser
{
    public static final String LOG_TAG = "SV3DConfigChooser";

    @Override
    public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display)
    {
        int attribs[] =
        {
            EGL10.EGL_SAMPLES, 4,
            EGL10.EGL_RED_SIZE, 8,
            EGL10.EGL_GREEN_SIZE, 8,
            EGL10.EGL_BLUE_SIZE, 8,
            EGL10.EGL_ALPHA_SIZE, 8,
            EGL10.EGL_DEPTH_SIZE, 8,
            EGL10.EGL_NONE
        };
        EGLConfig[] configs = new EGLConfig[1];
        int[] configCounts = new int[1];
        egl.eglChooseConfig(display, attribs, configs, 1, configCounts);

        if (configCounts[0] == 0)
        {
            Log.e(LOG_TAG, "No EGL config found");
            return null;
        }
        return configs[0];
    }
}

class SV3DSurfaceView extends GLSurfaceView
{
    public static final String LOG_TAG = "SV3DSurfaceView";

    private SV3DActivity mActivity;
    private SV3DRenderer mRenderer;

    public SV3DSurfaceView(SV3DActivity activity)
    {
        super(activity);
        mActivity = activity;

        this.setBackgroundColor(0x00000000);
        this.setEGLConfigChooser(new SV3DConfigChooser());
        this.getHolder().setFormat(PixelFormat.RGBA_8888);

        // Create an OpenGL ES 2.0 context
        setEGLContextClientVersion(2);

        // Set the Renderer for drawing on the GLSurfaceView
        mRenderer = new SV3DRenderer(mActivity);
        setRenderer(mRenderer);

        // Receive key event
        setFocusable(true);
    }

    public void SetApp(SV3DApp app)
    {
        mRenderer.SetApp(app);
    }

    public void ResetApp()
    {
        mRenderer.ResetApp();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event)
    {
        SV3DNative.OnTouchEvent(event);
        requestRender();

        return true;
    }

    @Override
    public boolean onGenericMotionEvent(MotionEvent event)
    {
        switch (event.getAction())
        {
            case MotionEvent.ACTION_SCROLL:
            {
                SV3DNative.OnTouchEvent(event);
                return true;
            }
        }

        return super.onTouchEvent(event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event)
    {
        if (SV3DNative.OnKeyUp(keyCode))
        {
            return true;
        }

        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event)
    {
        if (SV3DNative.OnKeyDown(keyCode))
        {
            return true;
        }

        return super.onKeyDown(keyCode, event);
    }
}