/*
*
* Copyright 2017,2022 NXP
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice (including the next
* paragraph) shall be included in all copies or substantial portions of the
* Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*/


#ifndef SRC_CONTOURS_HPP_
#define SRC_CONTOURS_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgproc/imgproc_c.h>

using namespace cv;
using namespace std;

/*******************************************************************************************
 * Macros
 *******************************************************************************************/
#define MAX_CONTOUR_APPROX  7
#define CONTOURS_NUM 4

#define MIN4(a,b,c,d)  (((a <= b) && (a <= c) && (a <= d)) ? (a) : \
						(((b <= c) && (b <= d)) ? (b) : \
						((c <= d) ? (c) : (d))))

#define MAX4(a,b,c,d)  (((a >= b) && (a >= c) && (a >= d)) ? (a) : \
						(((b >= c) && (b >= d)) ? (b) : \
						((c >= d) ? (c) : (d))))

/*******************************************************************************************
 * Types
 *******************************************************************************************/
struct CvContourEx
{
    CV_CONTOUR_FIELDS()
    int counter;
};

/*******************************************************************************************
 * Global functions
 *******************************************************************************************/
/**************************************************************************************************************
 *
 * @brief  			Search contours in input image
 *
 * @param  	in		const Mat &img - input image
 * 			in/out	CvSeq** root - sequence of contours
 * 			in		CvMemStorage *mem_storage - memory storage
 *			in		int min_size - empiric bound for minimal allowed perimeter for contour squares
 *
 * @return 			Functions returns the number of contours which were found.
 *
 * @remarks 		The function applies adaptive threshold on input image and searches contours in image.
 * 					If 4 contours are found then function has been terminated. Otherwise it changes block size
 * 					for adaptive threshold and tries again.
 *
 **************************************************************************************************************/
extern int GetContours(const Mat &img, CvSeq** root, CvMemStorage *mem_storage, int min_size);

/**************************************************************************************************************
 *
 * @brief  			Sort contours from left to right
 *
 * @param  	in		CvSeq** root - sequence of contours
 *
 * @return 			-
 *
 * @remarks 		The function searches min value of contour points in X axis and then sorts all contours
 * 					according to this value from left to right.
 *
 **************************************************************************************************************/
extern void SortContours(CvSeq** root);

/**************************************************************************************************************
 *
 * @brief  			Generate vector of contour points in proper order.
 *
 * @param  	in		CvSeq** root - sequence of contours
 * 			out		vector<Point2f> * feature_points
 * 			in		Point2f shift
 *
 * @return 			-
 *
 * @remarks 		The function sorts contours corners from top-left clockwise, applies shift on corners
 * 					coordinates and push the corners to the feature_points array in proper order.
 *
 **************************************************************************************************************/
extern void GetFeaturePoints(CvSeq** root, vector<Point2f> &feature_points, Point2f shift);

/**************************************************************************************************************
 *
 * @brief  			Filter found contours.
 *
 * @param  	in/out	CvSeq** root - sequence of contours
 *
 * @return 			-
 *
 * @remarks 		The function checks all contours from the input sequence and removes contours
 * 					which are not located inside another sequence contour or which do not contain
 * 					another sequence contour.
 *
 **************************************************************************************************************/
extern void FilterContours(CvSeq** root);

/**************************************************************************************************************
 *
 * @brief  			Convert contours sequence into the vector of points.
 *
 * @param  	in		CvSeq** root - sequence of contours
 * 			out		vector<Point2f> * feature_points
 * 			in		Point2f shift
 *
 * @return 			-
 *
 * @remarks 		The function convert sequence of contours into points array. The shift is applied on corners
 * 					coordinates.
 *
 **************************************************************************************************************/
extern void sec2vector(CvSeq** root, vector<Point2f> &feature_points, Point2f shift);

#endif /* SRC_CONTOURS_HPP_ */
