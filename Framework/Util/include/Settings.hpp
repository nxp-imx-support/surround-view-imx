/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

struct CamParam
{
    int height;         // Camera frame height in pixels
    int width;          // Camera frame width in pixels
    float sf;           // Defisheye scale factor
    int roi;            // Region of interest for contours searching in percent of input frame height from bottom
    int cntr_min_size;  // Minimum length of contour in pixels
    int chessboard_num; // Number of chessboard images which will be used for camera calibraton
    std::string source; // Device name (/dev/videoX)
    std::string format; // Video format
};

class Settings
{
public:
    Settings() = default;
    virtual ~Settings() = default;

    // Paths
    std::string cameraInputs;  // Path to camera calibration frame files
    std::string cameraModels;  // Path to camera model files
    std::string templateFiles; // Path to template points files

    // Camera parameters
    int camerasCount;
    bool dewarp = true;
    CamParam cameras[4];

    // Display
    int displayHeight;
    int displayWidth;
    bool showDebug;
    int maxFPS = 0; // If FPS is higher than MaxFPS, application sleeps to render at MaxFPS. 0 is unlimited.
    bool fullscreen = true;

    // Grid parameters
    int gridAngles;       // Every quadrant of circle will be divided into this number of arcs
    int gridStartAngle;   // The parameter sets a circle segment for which the grid will be generated.
                          // It defines 2 points of circle secant. The first point is located in
                          // I quadrant and it is the start point of start_angle arc. And the second point is
                          // located in II quadrant and it is the end point of (grid_angles - grid_start_angle) arc
    int gridPointsZCount; // Number of points in z axis
    float gridStepX;      // Step in x axis for bowl side which is used to define grid points in z axis.
                          // Step in z axis: step_z[i] = (i * step_x_2)^2, i = 1, 2, ... - number of point
    float bowlRadius;     // Bowl radius
    float smoothAngle;    // Mask angle of smoothing
    std::string keyboard; // Keyboard device
    std::string mouse;    // Mouse device
    std::string display;  // Display device
    float modelScale[3];  // Car model scale

    // Object Detection
    bool objDetEnable = false;
    int maxIPS = 0; // If IPS is higher than MaxFPS, application sleeps to make inference at MaxIPS. 0 is unlimited

    // Exposure Correction
    int ecRefreshRate = 0; // Compute exposure correction every EcRefreshRate frames. 0 is never.

    int ReadXML(const char* filename);
    // Get template width (maximum value of template vertices x coordinate)
    int GetTmpMaxVal(const char* filename, int* val);
    void PrintParam(void);

private:
    int GetParam(const char* name);
    int SetParam(int num, const char* val);

    void ReadBool(const char* src, bool* dst);
    int ReadUInt(const char* src, int* dst);
    int ReadFloat(const char* src, float* dst);
    int ReadCamera(const char* src, int index, CamParam* dst);
};
