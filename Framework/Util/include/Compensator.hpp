/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class Compensator
{
public:
    Compensator(cv::Size maskSize);

    /** @brief Get coordinates of rectangle reflection across the x-axis.
      * @param  index Rectangle index
      */
    cv::Rect GetFlipROI(uint index);

    /** @brief Fills Compensator properties.
      * Calculates mask which defines 4 overlap regions and circumscribed rectangles of each region.
      */
    void Feed(uint camerasCount, std::vector<std::vector<cv::Point3f>>& seamPoints);

    /** @brief Save compensator info
      * Generates grids only for overlap regions which lays on flat bowl bottom for each camera.
      * Saves the grids into "compensator" folder, 1 grid per camera.
      * Each grid contained description of two overlap regions: left and right.
      * Also the application save the file with texel coordinates of circumscribed rectangle for each overlap regions.
      * The texture mapping application uses this information to copy only overlap regions from frame buffer
      * when it calculates exposure correction coefficients.
      */
    int Save(const char* path);

    /** @brief Load compensator info
      * Loads texel coordinates of circumscribed rectangle for each overlap region.
      * The texture mapping application uses this information to copy only overlap regions from frame buffer
      * when it calculates exposure correction coefficients.
      */
    int Load(const char* path);

private:
    std::vector<cv::Rect2f> mROI;
    double mRadius;
    cv::Mat mMask;
};
