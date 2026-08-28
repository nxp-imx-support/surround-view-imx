/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <fstream>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "Camera.hpp"
#include "Line.hpp"

struct Seam
{
    Line line;      // Coefficients y = alpha * x + beta
    cv::Point2f p1; // Start point
    cv::Point2f p2; // End point
};

class Masks
{
public:
    /** @brief Copies elements i from mSeamLeft and mSeamRight vector to the left and right pointers. */
    int GetSeam(Seam* left, Seam* right, uint i);

    /** @brief Calculate masks for 3D BEV.
      * @param cameras Vector of Camera objects
      * @param seamPoints Vector of input seam points for all grids.
      * The grid edge consist of 8 points.
      * Points 1-4 describe the left edge of grid (they are located in II quadrant).
      * Points 5-8 describe right edge of grid (they are located in I quadrant).
      * - 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of y coordinate;
      * - 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge;
      * - 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
      * - 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
      * - 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
      * - 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
      * - 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
      * - 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of y coordinate.
      * @param smoothing Smoothing angle value
      * Calculates masks for 3D BEV. The masks will be used for texture mapping.
      * They must be defined for original captured image from camera (with fisheye istortion)
      * because the same transformation will be applied on camera frames and masks.
      * The procedure of mask calculation:
      * - Calculates seams for every two adjacent grids.
      * The seam of two adjacent grids is a line y = a * x + b.
      * Coefficients a and b are found from grid intersection.
      * The seam points are defined for 3D template and then are projected to an image plane using cv::projecPoints
      * cv::projectPoints function is used to extract extrinsic and intrinsic camera parameters:
      * camera matrix, distortion coefficients, rotation and translation vectors.
      * - Creates masks which are limited to seams.
      * All 2D seam points describe a convex polygon which defines the mask in 2D image.
      * The polygon is filled with white color, and background is filled with black color.
      * - Smoothes mask edges.
      */
    void CreateMasks(std::vector<std::shared_ptr<Camera>>& cameras, std::vector<std::vector<cv::Point3f>>& seamPoints,
        float smoothing);

    /** @brief Split grid into opaque and transparency grids
      * @return 0 if all grid files "arrayX" exists. -1 on failure.
      * Reads grid of texels/vertices from the "arrayX" file (X = 1,2,3,4 is camera number).
      * Produces one grid for overlapping regions ("arrayX1") and one grid for non-overlapping regions ("arrayX2").
      * Camera blending mask is used to split grid into two parts.
      * The mask value are checked for each texels of a rendered triangle.
      * If at least one of texels is masked with value less than 255 then the triangle is written to overlap grid.
      * Otherwise the triangle is written to non-overlap grid.
      */
    int SplitGrids(void);

private:
    std::vector<cv::Mat> mMasks;
    std::vector<Seam> mSeamLeft;
    std::vector<Seam> mSeamRight;

    /** @brief Smoothes mask
      * @param mask Input maks
      * @param colors Input range of colors [left edge color, right edge color]
      *               For left mask edge, range will be [0, 255]
      *               For right mask edge, range will be [255, 0]in
      * @param angles Angles used for smoothing in left and right directions from mask border.
      * @param angle_step Step of color gradient
      * @param   edgePoints Vector of template points. Must include 3 points:
      *                    1. The first point of seam (intersection of bottom grid borders, z = 0)
      *                    2. The intersection point of seam line and base circle (z = 0)
      *                    3. The 3rd point of seam points (with maximum value of z coordinate)
      * @param camera  Camera object
      * Smoothes the mask left and right edges to obtain a seamless blending of frames.
      * The angle of smoothing (2 * SMOTHING_ANGLE) defines the area in which mask edge will be smooth.
      * The original seam divides the smoothing angle into two angles with equal measures (angle bisector).
      * The angle based smoothing is applied only for flat base.
      * The seam at the bowl side is smoothed with constant width of smoothing.
      */
    void SmoothMaskEdge(cv::Mat& img, cv::Vec2b colors, cv::Vec2d angles, double angle_step,
        std::vector<cv::Point3f> edgePoints, std::shared_ptr<Camera> camera);

    /** @brief Get intersection of two polygons
      * @param polygon1 First convex polygon points. The polygon is described with 8 points:
      *                  4 ____ 5
      *                 3 /    \ 6
      *                  |      |
      *                 2 \____/ 7
      *                 1      8
      * @param polygon2 Second convex polygon points. The polygon is described with 8 points: 1st
      * point - left bottom vertex, 4th point - left top vertex, 5th point - right top vertex, 8th point - right bottom
      * point. The polygon must be convex. 
      * @param seam Output polygons intersection (2 points + line that goes through them)
      * @param rotation Direction of rotation for the second polygon.
      *                 If rotation < 0, then the second polygon is rotated on 90 degrees angle to the left.
      *                 Otherwise it is rotated on 90 degrees angle to the right.
      * Searches for two convex polygons intersection.
      * The first intersection point is searched between polygons bottom sides 1-2, 1-8, 7-8.
      * The second intersection point is searched between top sides 4-5.
      * The polygon intersection is calculated after second polygon points have been rotated.
      */
    void GetSeamPoints(std::vector<cv::Point3f>& polygon1, std::vector<cv::Point3f>& polygon2, Seam& seam,
        int rotation);
};
