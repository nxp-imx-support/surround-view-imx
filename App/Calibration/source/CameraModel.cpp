/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "CameraModel.hpp"
#include "Log.hpp"

inline float CameraModel::Lerp(float v0, float v1, float t)
{
    return (1.0f - t) * v0 + t * v1;
}

float CameraModel::MapAt(const cv::Mat& map, const cv::Point2f& coord)
{
    cv::Point ind((int)coord.x, (int)coord.y);
    cv::Point2f t = coord - cv::Point2f(ind.x, ind.y);

    int x1 = (ind.x + 1 < map.cols);
    int y1 = (ind.y + 1 < map.rows);

    if (ind.x < 0 || ind.y < 0 || ind.x + x1 >= map.cols || ind.y + y1 >= map.rows) {
        LogWarning("getMapAt: try to get coordinates out of bounds (%.2f, %.2f)", coord.x, coord.y);
        return 0.0f;
    }
    float v0 = Lerp(map.at<float>(ind), map.at<float>(ind + cv::Point(x1, 0)), t.x);
    float v1 = Lerp(map.at<float>(ind + cv::Point(0, y1)), map.at<float>(ind + cv::Point(x1, y1)), t.x);
    float v = Lerp(v0, v1, t.y);

    return v;
}

bool CameraModel::MapHas(const cv::Point2i& coord)
{
    return ((coord.x < mImgSize.width) && (coord.x >= 0) && (coord.y < mImgSize.height) && (coord.y >= 0));
}

bool CameraModel::MapHas(const cv::Point2f& coord)
{
    cv::Point p((int)round(coord.x), (int)round(coord.y));
    return MapHas(p);
}

void CameraModel::Remap(const cv::Mat& in, cv::Mat& out)
{
    cv::remap(in, out, mXmap, mYmap, (int)cv::INTER_LINEAR);
}

float CameraModel::XMapAt(const cv::Point2f& coord)
{
    return MapAt(mXmap, coord);
}

float CameraModel::YMapAt(const cv::Point2f& coord)
{
    return MapAt(mYmap, coord);
}

float CameraModel::XMapAt(const cv::Point2i& coord)
{
    return mXmap.at<float>(coord);
}

float CameraModel::YMapAt(const cv::Point2i& coord)
{
    return mYmap.at<float>(coord);
}

cv::Size CameraModel::Size()
{
    return mImgSize;
}

void CameraModel::SetScale(float scale)
{
    mScale = scale;
    UpdateLUT();
}
