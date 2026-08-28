/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "CameraModel.hpp"

#include <fstream>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <sys/stat.h>

class FisheyeModel : public CameraModel
{
public:
    //// @param filename - File with polynomial camera model from Scaramuzza toolbox
    FisheyeModel(std::string filename, float scaleFactor);
    virtual ~FisheyeModel() = default;

    void UpdateLUT();
    void Cam2World(cv::Point3d* p3d, cv::Point2d p2d);

private:
    // Polynomial coefficients of radial camera model
    std::vector<double> mPolCoefs;

    // Coefficients of the inverse polynomial
    std::vector<double> mInvPolCoefs;

    // Affine coefficients matrix
    // | sx  shy |
    // | shx sy  |
    // sx, sy - scale factors along the x/y axis
    // shx, shy - shear factors along the x/y axis
    cv::Matx22d mAffine;

    // Coordinates of the center in pixels
    cv::Point2d mCenter;
};
