/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>

class CameraModel
{
public:
    CameraModel() = default;
    virtual ~CameraModel() = default;

    virtual void UpdateLUT() = 0;

    void Remap(const cv::Mat& in, cv::Mat& out);
    bool MapHas(const cv::Point2i& coord);
    bool MapHas(const cv::Point2f& coord);

    float XMapAt(const cv::Point2i& coord);
    float YMapAt(const cv::Point2i& coord);
    float XMapAt(const cv::Point2f& coord);
    float YMapAt(const cv::Point2f& coord);
    float MapAt(const cv::Mat& map, const cv::Point2f& coord);

    cv::Size Size();
    void SetScale(float scale);

protected:
    inline float Lerp(float v0, float v1, float t);

    cv::Size mImgSize;
    float mScale;
    cv::Mat mXmap, mYmap;
};
