/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Camera.hpp"
#include "Macros.hpp"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

struct GridParam
{
    uint angles;       // Every quadrant of circle will be divided into this number of arcs
    uint startAngle;   // The mParams sets a circle segment for which the grid will be generated.
                       // It defines 2 points of circle secant. The first point is located in
                       // I quadrant and it is the start point of startAngle arc. And the second point is
                       // located in II quadrant and it is the end point of (angles - startAngle) arc
    uint zPointsCount; // Number of points in z axis
    double stepX;      // Step in x axis which is used to define grid points in z axis.
                       // Step in z axis: stepZ[i] = (i * stepX)^2, with i in [1, zPointsCount] range.
};

struct CameraInfo
{
    int width;
    int height;
    int index;
};

class Grid
{
public:
    Grid() = default;
    virtual ~Grid() = default;

    void SetAngles(uint val);
    void SetStartAngle(uint val);
    void SetNopZ(uint val);
    void SetStepX(double val);

    /** @brief Copy points from seam property into seamPoints */
    void GetSeamPoints(std::vector<cv::Point3f>& seamPoints);

    /** @brief Generates grid points for mesh generation.
      * @param points Output vertices. Array is allocated by the functions.
      */
    int GetGrid(float** points);

    /** @brief Generates 3D grid of texels/vertices for the input Camera object.
      * @param camera Camera object
      * @param radius Radius of base circle. The radius must be defined relative to template width.
      *               The template width (in pixels) is considered as 1.0.
      * Defines 3D grid, generates triangles from it and saves the triangles into file.
      */
    virtual void CreateGrid(std::shared_ptr<Camera> camera, double radius) = 0;

    /** @brief Generates triangles from a 3D grid and save them to a file.
      * @param camera Camera object
      * The file with triangles description is saved to the file arrayX, where X is camera index.
      * The grid is rotated according to the camera index value: (index * 90 degrees clockwise rotation)
      */
    virtual void SaveGrid(std::shared_ptr<Camera> camera) = 0;

protected:
    /** @brief Find seam points.
      * @param camera Camera object
      * @param seamPoints Output seam points
      * Search 8 points to define the edge of grid.
      * If the seamPoints vector size is not 8, there was a problem with points searching and result of function is inapplicable.
      * Points 1-4 describe the left edge of grid (they are located in II quadrant).
      * - 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of y coordinate;
      * - 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge;
      * - 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
      * - 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
      * Points 5-8 describe right edge of grid (they are located in I quadrant).
      * - 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
      * - 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
      * - 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
      * - 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of y coordinate.
      */
    virtual void FindSeam(std::shared_ptr<Camera> camera, std::vector<cv::Point3f>& seamPoints) = 0;

    /** @brief Rotate point according to the camera index
      * Rotate grid point according to the camera index value: (index * 90 degrees clockwise rotation)
      */
    cv::Point3f RotatePoint(int index, cv::Point3f point);

    CameraInfo mCamInfo;
    std::vector<cv::Point3f> mP3D; // 3D template points
    std::vector<cv::Point2f> mP2D; // 2D image points
    GridParam mParams;
    std::vector<cv::Point3f> mSeamPoints;
};

/** @class CurvilinearGrid
  * Grid is denser at the middle of bowl bottom and more sparse at the bowl bottom edge.
  */
class CurvilinearGrid : public Grid
{
public:
    /** @param angles Every quadrant of circle will be divided into this number of arcs.
      * @param startAngle It is not necessary to define grid through a whole semi-circle.
      *                   The startAngle sets a circular sector for which the grid will be generated.
      * @param zPointsCount Number of points in z axis.
      * @param stepX Grid step in polar coordinate system.
      *              The value is also used to define grid points in z axis.
      *              Step in z axis: stepZ[i] = (i * stepX)^2, with i in [1, zPointsCount] range.
      */
    CurvilinearGrid(uint angles, uint startAngle, uint zPointsCount, double stepX);

    void CreateGrid(std::shared_ptr<Camera> camera, double radius) override;
    void SaveGrid(std::shared_ptr<Camera> camera) override;

private:
    int mPointsCount; // Number of grid points for one grid sector (angle)

    /** @brief Reorganize grid to clean middle part of view.
      * @param camera Camera object
      * @param radius Radius of base circle. The radius must be defined relative to template width.
      *               The template width (in pixels) is considered as 1.0.
      * Reorganizes input 3d and 2d grids to clean middle part of view.
      * Replaces all points which are defined on 3d grid but aren't defined on camera frame with border values.
      */
    void ReorgGrid(double radius, std::shared_ptr<Camera> camera);

    void FindSeam(std::shared_ptr<Camera> camera, std::vector<cv::Point3f>& seamPoints) override;
};

/** @class RectilinearGrid
  * Grid is sparse at the middle of bowl bottom and more denser at the bowl bottom edge.
  * To fill circle base with vertices grid the uneven grid is used.
  * The circle is divided into arcs of same values.
  * The intersection points of arcs define X and Y coordinates of grid corners.
  * In this case the grid at the edge of circle is denser than at the center.
  */
class RectilinearGrid : public Grid
{
public:
    /** @param angles Every quadrant of circle will be divided into this number of arcs.
      * @param startAngle It is not necessary to define grid through a whole semi-circle.
      *                   The startAngle sets a circle segment for which the grid will be generated.
      *                   The parameter defines 2 points of circle secant.
      *                   The first point is located in I quadrant and it is the start point of startAngle arc.
      *                   The second point is located in II quadrant and it is the end point of (angles - startAngle) arc.
      * @param zPointsCount Number of points in z axis.
      * @param stepX Step in x axis which is used to define grid points in z axis.
      *              Step in z axis: stepZ[i] = (i * stepX)^2, with i in [1, zPointsCount] range.
      */
    RectilinearGrid(uint angles, uint startAngle, uint zPointsCount, double stepX);

    void CreateGrid(std::shared_ptr<Camera> camera, double radius) override;
    void SaveGrid(std::shared_ptr<Camera> camera) override;

private:
    std::vector<int> mPointsCount; // Number of points in grid column

    void FindSeam(std::shared_ptr<Camera> camera, std::vector<cv::Point3f>& seamPoints) override;
};
