/*
 * Copyright 2017 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>

#define MAX_CONTOUR_APPROX 7
#define CONTOURS_NUM 4

#define MIN4(a, b, c, d) \
    (((a <= b) && (a <= c) && (a <= d)) ? (a) : (((b <= c) && (b <= d)) ? (b) : ((c <= d) ? (c) : (d))))

#define MAX4(a, b, c, d) \
    (((a >= b) && (a >= c) && (a >= d)) ? (a) : (((b >= c) && (b >= d)) ? (b) : ((c >= d) ? (c) : (d))))

struct CvContourEx
{
    CV_CONTOUR_FIELDS()
    int counter;
};

/** @brief Searchs contours in input image.
  * @param img Input image.
  * @param root Sequence of contours.
  * @param memStorage Memory storage.
  * @param minSize Empiric bound for minimal allowed perimeter for contour squares.
  * @return Number of contours found.
  * Applies adaptive threshold on input image and searches contours in image.
  * If 4 contours are found then function has been terminated.
  * Otherwise it changes block size for adaptive threshold and tries again.
  */
extern int GetContours(const cv::Mat& img, CvSeq** root, CvMemStorage* memStorage, int minSize);

/** @brief Sort contours from left to right.
  * @param root Sequence of contours.
  * Searches min value of contour points in X axis,
  * and then sorts all contours according to this value from left to right.
  */
extern void SortContours(CvSeq** root);

/** @brief Generates vector of contour points in proper order.
  * @param root Sequence of contours
  * @param featurePoints Output feature points
  * @param shift
  * Sorts contours corners from top-left clockwise, applies shift on corners coordinates
  * and push the corners to the featurePoints array in proper order.
  */
extern void GetFeaturePoints(CvSeq** root, std::vector<cv::Point2f>& featurePoints, cv::Point2f shift);

/** @brief Filters found contours.
  * @param root Sequence of contours
  * Checks all contours from the input sequence
  * and removes contours which are not located inside another sequence contour
  * or which do not contain another sequence contour.
  */
extern void FilterContours(CvSeq** root);

/** @brief Converts contours sequence into the vector of points.
  * @param root Sequence of contours
  * @param featurePoints Output feature points
  * @param shift
  * Convert sequence of contours into points array. The shift is applied on corners coordinates.
  */
extern void SequenceToVector(CvSeq** root, std::vector<cv::Point2f>& featurePoints, cv::Point2f shift);
