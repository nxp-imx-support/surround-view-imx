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


#ifndef SRC_CAMERA_HPP_
#define SRC_CAMERA_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include "defisheye.hpp"
#include "src_contours.hpp"

using namespace cv;
using namespace std;

/*******************************************************************************************
 * Types
 *******************************************************************************************/
struct CameraTemplate { 		/* Template parameters */
	char filename[50];			/* Name of file with coordinates of reference points */
	Size size;					/* Template size (height, width) in pixels */
	uint pt_count;				/* Reference points number */
	vector<Point3f> ref_points; /* Reference points */
};

struct CameraParameters { 		/* Intrinsic and extrinsic camera parameters */
	Mat K; 						/* Camera matrix */
	Mat distCoeffs; 			/* Distortion coefficients */
	Mat rvec;					/* Rotation vector */
	Mat tvec;					/* Translation vector */
};

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* Camera class */
class Camera {
	public:
		Mat xmap, ymap;			/* X and Y maps for removing fisheye distortion */
    	int index;				/* Camera index */
    	Defisheye model;		/* Polynomial camera model */
    	float sf;				/* Scale factor */
    	Size poster;			/* The calibrating poster size */
    	CameraTemplate tmp;		/* Template parameters */

    	Mat getK(void) {Mat M; param.K.copyTo(M); return M;} // Get camera matrix
    	Mat getDistCoeffs(void) {Mat M; param.distCoeffs.copyTo(M); return M;} // Get distortion coefficients
    	Mat getRvec(void) {Mat M; param.rvec.copyTo(M); return M;} // Get rotation vector
    	Mat getTvec(void) {Mat M; param.tvec.copyTo(M); return M;} // Get translation vector

	/**************************************************************************************************************
	 *
	 * @brief  			Update LUTs for removing defisheye distortion.
	 *
	 * @param  in 		float scale_factor - scale factor.
	 *
	 * @return 			-
	 *
	 * @remarks 		The function recalculates LUTs for removing defisheye distortion with new value of scale factor.
	 *
	 **************************************************************************************************************/
	void updateLUT(float scale_factor);	
		
	/**************************************************************************************************************
	 *
	 * @brief  			Convert vector of points into array of lines
	 *
	 * @param  out 		float** lines - array of lines
	 *
	 * @return 			Functions returns number of array elements.
	 *
	 * @remarks			The function converts vector of contours points into array of lines. Input data consist of  
	 *					contours points in 2D space (x,y coordinates). Output array consist of lines in 3D space.
	 *					Each line is described as 6 floats - 2 3D coordinates of its edges. If a point is a part of 
	 *					two or more lines, its coordinates will be added to output array more times.
	 *
	 **************************************************************************************************************/	
	int getContours(float** lines);	
			
    	/**********************************************************************************************************
    	 *
     	 * @brief  		Get base radius
     	 *
     	 * @param   	-
     	 *
    	 * @return 		double - the function returns value of private class attribute radius which is defined as
    	 * 				the radius of the circle which is circumscribed around the calibration poster in pixels.
    	 * 				The value is calculated from poster size as a half of it diagonal.
    	 *
    	 **********************************************************************************************************/
    	double getBaseRadius(void) {return radius;}

    	/**********************************************************************************************************
    	 *
    	 * @brief  		Define the region of interest for contours searching
    	 *
    	 * @param  in  	int val - region of interest in which pattern contours will be searched.
    	 *							It is defined in percent of input frame height from bottom.
    	 *
    	 * @return 		-
    	 *
    	 **********************************************************************************************************/
	void setRoi(int val) {if((uint)val > 100U) { roi = 1.0f; } else {roi = (float)val / 100.0f;} }

	/**********************************************************************************************************
	 *
	 * @brief  		Set empiric bound for minimal allowed perimeter for contour squares
	 *
	 * @param  in  	int val - empiric bound for minimal allowed perimeter for contour squares.
	 *
	 * @return 		The private attribute cntr_min_size is set.
	 *
	 **********************************************************************************************************/
	void setContourMinSize(int val) {cntr_min_size = val; } // Empiric bound for minimal allowed perimeter for contour squares

	/*******************************************************************************
	* @brief  		Remove fisheye distortion from image
	* @param  in  	Mat &img - input fisheye image
	*		 out	Mat &out - output undistorted image.
	* @return 		-
	*******************************************************************************/
    	void defisheye(Mat &img, Mat &out) {remap(img, out, xmap, ymap, (int)cv::INTER_LINEAR);}

    	/**************************************************************************************************************
    	 *
    	 * @brief  		Set template parameters.
    	 *
    	 * @param  in 		char *filename - *.txt file with coordinates of reference points
    	 * 					Size template_size - the size of global template for whole 4 cameras system
    	 *
    	 * @return 			Functions returns 0 if the file-status information is obtained. If the file not found
    	 * 					the function returns -1.
    	 * 					The private property tmp of Camera object is set.
    	 *
    	 * @remarks 		The method sets template parameters (template file name, template size, number of template
    	 * 					points) if file filename exists.
    	 * 					The template size is used to normalize reference template point. For front and back cameras
    	 * 					the tmp.size property (which contain template size) is equal input value of template_size.
    	 * 					But for left and right cameras the template_size has been rotated.
    	 *
    	 **************************************************************************************************************/
     	int setTemplate(const char *filename, Size template_size);

     	/***************************************************************************************************************\
     	 *
     	 * @brief  		Set camera intrinsic parameters (camera matrix and distortion coefficients).
     	 *
     	 * @param  in 	char *filename - path to the folder which contains chessboard images.
     	 *				char *filename - chessboard *.jpg file base name.
     	 *				int img_num - number of calibrating images.
     	 * 				Size patternSize - number of chessboard corners in horizontal and vertical directions.
     	 *
     	 * @return 		The function returns 0 value if all chessboard corners were found and they are placed in a certain
     	 * 				order (row by row, left to right in every row). Otherwise -1 has been returned.
     	 * 				The private property param and public properties xmap, ymap of Camera object is set.
     	 *
     	 * @remarks		The intrinsic camera parameters are calculated for the camera after removing fisheye transformation.
     	 * 				Therefore the estimation of intrinsic camera parameters has been made after fisheye distortion has been removed.
     	 *				The function calculates camera matrix K using images of chessboard.
     	 *				    |fx   0   cx |
     	 *				K = |0    fy  cy |	(cx, cy) -   principal point at the image center
     	 *			    	|0    0    0 |	fx, fy - focal lengths in x and y axis
     	 * 				Distortion coefficients distCoeffs are set to 0 after defisheye transformation.
     	 *				LUTs are calculated.
     	 *
     	 *************************************************************************************************************/
	int setIntrinsic(const char *filepath, const char *filename, int img_num, Size patternSize);

     	/**************************************************************************************************************
     	 *
     	 * @brief  			Set camera extrinsic parameters (rotation and translation vectors).
     	 *
     	 * @param  in 		const Mat &img - captured calibrating frame from camera
     	 *
     	 * @return 			Functions returns 0 if camera extrinsic parameters have been set. A return value of - 1
     	 * 					indicates some problems.
     	 *					The private properties param and radius of Camera object are set.
     	 *
     	 * @remarks 		The function calculate extrinsic camera parameters finding an object pose from 3D-2D point
     	 * 					correspondences.
     	 * 					To estimate extrinsic parameters a special template with patterns of knowing size must be used.
     	 * 					The application identifies all pattern corners in the captured camera images and establishes
     	 * 					a correspondence with the real world distance of these corners. Using these correspondences
     	 * 					the extrinsic camera parameters (rotation and translation vectors) have been estimated.
     	 *					The estimation of extrinsic parameters has been made after fisheye distortion has been removed.
     	 *					It is necessary to set templates parameters and intrinsic camera parameters before the extrinsic
     	 *					camera parameters will be calculated.
     	 *
     	 *					Important: calibration patterns must be visible on the captured frame.
     	 **************************************************************************************************************/
     	int setExtrinsic(const Mat &img);

    	/**************************************************************************************************************
    	 *
    	 * @brief  			Get maximum number of grid rows in z axis.
    	 *
    	 * @param  in 		double radius - radius of flat circle bottom of bowl. The radius must be defined relative
    	 * 									to template width. The template width (in pixels) is considered as 1.0.
    	 * 		   in		double step_x -	step in x axis which is used to define grid points in z axis.
		 * 									Step in z axis: step_z[i] = (i * step_x)^2, i = 1, 2, ... - number of point.
    	 *
    	 * @return 			Functions returns the maximum number of grid rows in z axis which can be rendered for defined
    	 * 					radius and step_x. It means that if we add one more grid row in z axis, then vertexes of this
    	 * 					row will not belong to the input camera frame. It will be outside of the camera FOV.
    	 *
    	 * @remarks 		The output is calculated for the input values of radius and step.
    	 *
    	 **************************************************************************************************************/
    	int getBowlHeight(double radius, double step_x);

	private:
		float roi;				/* ROI for contours finding */
		int cntr_min_size;		/* Empiric bound for minimal allowed perimeter for contour squares */
		double radius;			/* Base radius - the distance between template center and farthest point from the center */
		vector<Point2f> img_p;
    	CameraParameters param;	/* Camera parameters */
    	/**************************************************************************************************************
    	 *
    	 * @brief  			Search pattern points in captured image from the camera.
    	 *
    	 * @param  in		Mat &undist_img - defisheye captured image from the camera
    	 * 		   in		uint num - expected number of reference points
    	 * 		   out 		vector<Point2f> &img_points - vector of image points
    	 *
    	 * @return 			Functions returns 0 if points have been found successfully. A return value of - 1 indicates
    	 * 					an error.
    	 *
    	 * @remarks			The function searches contours in the bottom half of the input image. If 4 quadrangles have been
    	 * 					found successfully then contours have been sorted from left to right. Corners of each contour
    	 * 					have been sorted from top-left clockwise.
    	 * 					Sorted corners have been written to the img_points vector.
    	 *
    	 **************************************************************************************************************/
    	int getImagePoints(Mat &undist_img, uint num, vector<Point2f> &img_points);
};


/* Camera class creator */
class Creator {
	public:
	/**************************************************************************************************************
	 *
	 * @brief  			Camera class creator.
	 *
	 * @param  in 		char *filename - *.txt file with polynomial camera model from Scaramuzza toolbox for Matlab
	 * 		   in		float sf - scale factor
	 * 		   in 	 	int index - camera index
	 *
	 * @return 			Functions returns Camera* object if the camera model has been loaded successfully. A return
	 * 					value of NULL indicates an error.
	 * 					The public properties of Camera object are set: sf, index.
	 * 					The roi value is set on default value - 50%.
	 *
	 * @remarks 		The class creator reads polynomial camera model from filename file. If any problems were
	 * 					to occur then creator return NULL. Otherwise it returns the pointer to Camera object.
	 *
	 **************************************************************************************************************/
		static Camera* create(const char *filename, float sf, int index); /* Create Camera object */
};

#endif /* SRC_CAMERA_HPP_ */
