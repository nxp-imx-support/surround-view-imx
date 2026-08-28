/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "RectilinearModel.hpp"

RectilinearModel::RectilinearModel(uint32_t width, uint32_t height)
{
    mImgSize = cv::Size(width, height);
    UpdateLUT();
}

void RectilinearModel::UpdateLUT()
{
    mXmap.create(mImgSize.height, mImgSize.width, CV_32FC1);
    mYmap.create(mImgSize.height, mImgSize.width, CV_32FC1);

    for (int col = 0; col < mImgSize.width; col++) {
        for (int row = 0; row < mImgSize.height; row++) {
            mXmap.at<float>(row, col) = (float)col;
            mYmap.at<float>(row, col) = (float)row;
        }
    }
}
