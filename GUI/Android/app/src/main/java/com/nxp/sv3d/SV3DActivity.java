/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

package com.nxp.sv3d;

import android.util.Log;

import android.content.Context;
import android.content.pm.PackageManager;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.ActionBar;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import android.os.Bundle;
import android.os.Handler;
import android.annotation.SuppressLint;
import android.Manifest;

import android.view.View;
import android.view.KeyEvent;
import android.widget.TextView;
import android.widget.FrameLayout;
import android.widget.Button;
import android.widget.Toast;

import android.view.Menu;
import android.view.MenuItem;
import android.app.AlertDialog;
import android.content.DialogInterface;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;

import android.hardware.camera2.CameraManager;
import android.hardware.camera2.CameraCharacteristics;

public class SV3DActivity extends AppCompatActivity
{
    public static final String LOG_TAG = "SV3DActivity";

    private static Context mContext;
    public static Context getContext() {return mContext;}

    private static final int CAMERA_PERMISSION_REQUEST_CODE = 100;

    // Some older devices needs a small delay between UI widget updates
    // and a change of the status and navigation bar.
    private static final int UI_ANIMATION_DELAY = 300;

    // Views
    private FrameLayout mContentView;
    private View mControlsView;
    private FrameLayout mSurfaceViewContainer;
    SV3DSurfaceView mSurfaceView;
    private boolean mVisible;
    //private FrameLayout mSettingsView;

    // Buttons
    private Button mSettingsButton;
    private Button mCapturingButton;
    private Button mCalibrationButton;
    private Button mRenderButton;
    private Button mMenuButton;
    private Button mOkButton;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        mContext = this;

        setContentView(R.layout.activity_main);

        mControlsView = findViewById(R.id.content_controls);
        mContentView = findViewById(R.id.layout_fullscreen);
        mSurfaceViewContainer = findViewById(R.id.surface_view_container);

        // Create SV3D SurfaceView
        mSurfaceView = new SV3DSurfaceView(this);
        mSurfaceViewContainer.addView(mSurfaceView, 0);
        mSurfaceView.setVisibility(View.GONE);

        InitializeButtons();

        // Check camera permission
        checkCameraPermission();

        show();
    }

    @Override
    protected void onResume()
    {
        super.onResume();
        show();
    }

    private void checkCameraPermission() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.CAMERA) != PackageManager.PERMISSION_GRANTED) {
            // Permission is not granted, request it
            ActivityCompat.requestPermissions(this, new String[]{Manifest.permission.CAMERA}, CAMERA_PERMISSION_REQUEST_CODE);
        } else {
            // Permission already granted
            Log.d(LOG_TAG, "Camera permission already granted");
            SV3DNative.OnCameraPermissionGranted();
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        
        if (requestCode == CAMERA_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Permission granted
                Log.d(LOG_TAG, "Camera permission granted");
                Toast.makeText(this, "Camera permission granted", Toast.LENGTH_SHORT).show();
                SV3DNative.OnCameraPermissionGranted();
            } else {
                // Permission denied
                Log.e(LOG_TAG, "Camera permission denied");
                Toast.makeText(this, "Camera permission is required for this app", Toast.LENGTH_LONG).show();
                
                // Show explanation dialog
                new AlertDialog.Builder(this)
                    .setTitle("Camera Permission Required")
                    .setMessage("This app requires camera permission to function properly. Please grant the permission in app settings.")
                    .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            // Optionally, you can finish the activity if permission is critical
                            // finish();
                        }
                    })
                    .setNegativeButton("Retry", new DialogInterface.OnClickListener() {
                        @Override
                        public void onClick(DialogInterface dialog, int which) {
                            checkCameraPermission();
                        }
                    })
                    .show();
            }
        }
    }

    private void InitializeButtons() {
        // Capturing
        mCapturingButton = (Button) findViewById(R.id.button_capturing);
        mCapturingButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mSurfaceView.SetApp(SV3DApp.CAPTURING);
                mSurfaceView.setVisibility(View.VISIBLE);
                hide();
                Toast.makeText(getContext(),
                    "Image will be captured into " + getFilesDir().getPath() + "/Generated/CamCapture",
                    Toast.LENGTH_SHORT).show();
            }
        });

        // Calibration
        mCalibrationButton = (Button) findViewById(R.id.button_calibration);
        mCalibrationButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mSurfaceView.SetApp(SV3DApp.CALIBRATION);
                mSurfaceView.setVisibility(View.VISIBLE);
                hide();
            }
        });

        // Render
        mRenderButton = (Button) findViewById(R.id.button_render);
        mRenderButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mSurfaceView.SetApp(SV3DApp.RENDER);
                mSurfaceView.setVisibility(View.VISIBLE);
                hide();
            }
        });

        // Menu
        mMenuButton = (Button) findViewById(R.id.button_menu);
        mMenuButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                mSurfaceView.setVisibility(View.GONE);
                show();
            }
        });
        // Do not catch keyboard events
        mMenuButton.setFocusable(false);

        // Dummy
        Button dummyButton = (Button) findViewById(R.id.dummy);
        dummyButton.setFocusable(false);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.layout.action_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int itemId = item.getItemId();
        if (itemId == R.id.item_reload) {
            // Reload XML file
            Log.d(LOG_TAG, "Reload XML file");
            SV3DNative.ReloadXmlFile();
            mSurfaceView.ResetApp();
            Toast.makeText(getContext(), "XML file reloaded",
                Toast.LENGTH_SHORT).show();
            return true;
        }
        if (itemId == R.id.item_show_path) {
            // Show Content directory path
            Log.d(LOG_TAG, "Show Content directory path");
            AlertDialog.Builder alertDialogBuilder = new AlertDialog.Builder(this);
            alertDialogBuilder.setMessage("Path to Content and Generated folders:\n" + getFilesDir().getPath());
            alertDialogBuilder.setPositiveButton("OK",
               new DialogInterface.OnClickListener() {
               @Override
               public void onClick(DialogInterface arg0, int arg1) {}
            });
            AlertDialog alertDialog = alertDialogBuilder.create();
            alertDialog.show();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    private void hide() {
        Log.d(LOG_TAG, "hide");
        // Hide UI first
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.hide();
        }
        mControlsView.setVisibility(View.GONE);
        mVisible = false;

        // Schedule a runnable to remove the status and navigation bar after a delay
        mHideHandler.removeCallbacks(mShowPart2Runnable);
        mHideHandler.postDelayed(mHidePart2Runnable, UI_ANIMATION_DELAY);
    }

    private final Runnable mHidePart2Runnable = new Runnable() {
        @SuppressLint("InlinedApi")
        @Override
        public void run() {
            // Delayed removal of status and navigation bar

            // Note that some of these constants are new as of API 16 (Jelly Bean)
            // and API 19 (KitKat). It is safe to use them, as they are inlined
            // at compile-time and do nothing on earlier devices.
            mContentView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LOW_PROFILE
                    // Don't loose focus
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                    // Disable resizing
                    | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                    // Hide nav and status bars
                    | View.SYSTEM_UI_FLAG_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        }
    };

    private void show() {
        Log.d(LOG_TAG, "show");
        // Show the system bar
        mContentView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);

        mVisible = true;

        // Schedule a runnable to display UI elements after a delay
        mHideHandler.removeCallbacks(mHidePart2Runnable);
        mHideHandler.postDelayed(mShowPart2Runnable, UI_ANIMATION_DELAY);
    }

    private final Runnable mShowPart2Runnable = new Runnable() {
        @Override
        public void run() {
            // Delayed display of UI elements
            ActionBar actionBar = getSupportActionBar();
            if (actionBar != null) {
                actionBar.show();
            }
            mControlsView.setVisibility(View.VISIBLE);
        }
    };

    private final Handler mHideHandler = new Handler();
    private final Runnable mHideRunnable = new Runnable() {
        @Override
        public void run() {
            hide();
        }
    };

    // Schedules a call to hide() in delay milliseconds,
    // canceling any previously scheduled calls.
    private void delayedHide(int delayMillis) {
        mHideHandler.removeCallbacks(mHideRunnable);
        mHideHandler.postDelayed(mHideRunnable, delayMillis);
    }

    static {
        System.loadLibrary("@BINARY_NAME@");
    }
}