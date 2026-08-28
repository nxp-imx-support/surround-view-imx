/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>

#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "CameraModel.hpp"
#include "Contours.hpp"

// Template parameters
struct CameraTemplate
{
    std::string filename;                // Name of file with coordinates of reference points
    cv::Size size;                       // Template size (height, width) in pixels
    uint pt_count;                       // Reference points number
    std::vector<cv::Point3f> ref_points; // Reference points
};

// Intrinsic and extrinsic camera parameters
struct CameraParameters
{
    cv::Mat K;          // Camera matrix
    cv::Mat distCoeffs; // Distortion coefficients
    cv::Mat rvec;       // Rotation
    cv::Mat tvec;       // Translation
};

class Camera
{
public:
    Camera(std::shared_ptr<CameraModel> model, int index);
    virtual ~Camera() = default;

    cv::Mat GetK();
    cv::Mat GetDistCoeffs();
    cv::Mat GetRvec();
    cv::Mat GetTvec();
    std::shared_ptr<CameraModel> GetModel();
    int GetIndex();
    const CameraTemplate& GetTemplate();

    /** @brief Recalculates LUTs applying camera model with new value of scale factor.
      * Removing defisheye distortion for Fisheye camera model.
      */
    void UpdateLUT(float scaleFactor);

    /** @brief Converts vector of points into array of lines.
      * @param lines An array of lines, allocated by the function.
      * @return The number of elements in the array.
      * Input data consist of contours points in 2D space (x,y coordinates).
      * Output array consist of lines in 3D space. Each line is described as six floats - two 3D coordinates of its edges.
      * If a point is a part of two or more lines, its coordinates will be added to output array more times.
      */
    int CreateContours(float** lines);

    /** @brief Creates vertices from LUTs.
      * @param vertices An array of output vertices, allocated by the function.
      * @param density Distance in pixels between vertices.
      * @param filename Optional filename to save vertices to file (for debugging purposes).
      * @return The number of elements in the array.
      */
    int CreateVertices(float** vertices, int density, std::string filename = "");

    /** @brief Get base radius.
      * @return The radius of the circle which is circumscribed around the calibration poster in pixels.
      * The value is calculated from poster size as a half of it diagonal.
      */
    double GetBaseRadius(void) { return mRadius; }

    /** @brief Set the region of interest for contours searching.
      * @param roi region of interest in which pattern contours will be searched.
      * It is defined in percent of input frame height from bottom.
      */
    void SetRoi(int roi);

    /** @brief Set empiric bound for minimal allowed perimeter for contour squares. */
    void SetContourMinSize(int size);

    /** @brief Set template parameters.
      * @param filename .txt file with coordinates of reference points.
      * @param template_size the size of global template for whole 4 cameras system
      * @return 0 if succesful. -1 if the file not found
      * Sets template parameters (file name, size, number of points).
      * The template size is used to normalize reference template point.
      * For front and back cameras, template size is equal to templateSize.
      * For left and right cameras, the templateSize has been rotated.
      */
    int SetTemplate(std::string filename, cv::Size templateSize);

    /** @brief Set camera intrinsic parameters (camera matrix and distortion coefficients).
      * @param filepath Path to the folder which contains chessboard images
      * @param filename Chessboard file prefix without index and without *.jpg extension
      * @param imageCount Number of calibration images
      * @param patternSize Number of chessboard corners in horizontal and vertical directions
      * @return 0 if all of the corners are found. -1 on failure.
      * Intrinsic camera parameters are calculated for the camera after removing fisheye transformation.
      * Therefore the estimation of intrinsic camera parameters has been made after fisheye distortion has been removed.
      * Calculates camera matrix K using images of chessboard.
      *     |fx   0   cx |
      * K = |0    fy  cy |
      *     |0    0    0 |
      * (cx, cy) is principal point at the image center.
      * (fx, fy) is focal lengths in x and y axis.
      * Sets distortion coefficients to 0 after defisheye transformation.
      * Calculates LUTs.
      */
    int SetIntrinsic(std::string filepath, std::string filename, int imageCount, cv::Size patternSize);

    /** @brief Set camera extrinsic parameters (rotation and translation).
      * @param img Captured calibrating frame from camera.
      * @return 0 if camera extrinsic parameters have been set. -1 on failure.
      * Calculate extrinsic camera parameters finding an object pose from 3D-2D point correspondences.
      * To estimate extrinsic parameters a special template with patterns of knowing size must be used.
      * The application identifies all pattern corners in the captured camera images
      * and establishes a correspondence with the real world distance of these corners.
      * Using these correspondences, the extrinsic camera parameters (rotation and translation) are estimated.
      * The estimation of extrinsic parameters are made after fisheye distortion has been removed.
      * Requires templates and intrinsic parameters to be set before.
      * Calibration patterns must be visible on the captured frame.
      */
    int SetExtrinsic(const cv::Mat& img);

    /** @brief Get maximum number of grid rows in z axis.
      * @param radius Radius of flat circle bottom of bowl. The radius must be defined relative to template width. 
      *               The template width (in pixels) is considered as 1.0. in        double  -
      * @param stepX Step in x axis which is used to define grid points in z axis.
      *               Step in z axis: step_z[i] = (i * stepX)^2, i in [1, number of point] range.
      * @return The maximum number of grid rows in z axis which can be rendered for defined radius and stepX.
      * If one more grid row is added in z axis, then vertexes of this row will not belong to the input camera frame.
      * It will be outside of the camera FOV.
      */
    int GetBowlHeight(double radius, double stepX);

    static inline float ToClipSpaceX(float x) { return x * 2.0f - 1.0f; }
    static inline float ToClipSpaceY(float y) { return -y * 2.0f + 1.0f; }

private:
    /** @brief Search pattern points in captured image from the camera.
      * @param undistImg Defisheye captured image from the camera
      * @param num Expected number of reference points
      * @param img_points Image points
      * @return 0 if points have been found successfully. -1 on failure.
      * Searches contours in the bottom half of the input image.
      * If 4 quadrangles have been found successfully then contours have been sorted from left to right.
      * Corners of each contour have been sorted from top-left clockwise.
      * Sorted corners have been written to the img_points std::vector.
      */
    int GetImagePoints(cv::Mat& undistImg, uint num, std::vector<cv::Point2f>& img_points);

    float mRoi;          // ROI for contours finding
    int mContourMinSize; // Empiric bound for minimal allowed perimeter for contour squares
    double mRadius;      // Distance between template center and farthest point from the center

    std::vector<cv::Point2f> mImagePoints;
    CameraParameters mParams;
    cv::Size mPosterSize;
    std::shared_ptr<CameraModel> mModel;
    CameraTemplate mTemplate;
    int mIndex;
};
