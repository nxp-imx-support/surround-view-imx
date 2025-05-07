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


#ifndef SRC_MASKS_HPP_
#define SRC_MASKS_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <fstream>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camera.hpp"
#include "lines.hpp"
#include "macros.hpp"

using namespace cv;
using namespace std;

/*******************************************************************************************
 * Macros
 *******************************************************************************************/
#define SEAM_STEP 0.001

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* Masks class */
class Masks {
	public:
		/***********************************************************************************************************
		 *
		 * @brief  		Get seam.
		 *
		 * @param  out 	Seam* left - left seam.
		 * 				Seam* right - right seam.
		 * 				in	uint i - number of seam element.
		 *
		 * @return 		The function returns 1, if vector<Seam> seaml and
		 *				vector<Seam> seamr sizes are larger than i. Otherwise 0 is returned.
		 *
		 * @remarks 	The function copies elements i from seaml and seamr vectors (left and right seam)
		 *				to the left and right pointers.
		 ************************************************************************************************************/
		int getSeam(Seam* left, Seam* right, uint i) { if((i < seaml.size()) && (i < seamr.size()))
													   {
															memcpy(left, &seaml[i], sizeof(Seam));
															memcpy(right, &seamr[i], sizeof(Seam));
															return(1);
													   }
													   return(0);
													  }


		/**************************************************************************************************************
		 *
		 * @brief  			Calculate masks for 3D BEV.
		 *
		 * @param  	in		vector<Camera*> cameras - vector of Camera objects
		 * 			in		vector< vector<Point3f> > &seam_points - pointer to the vector containing seam points for all grids
		 * 					The grid edge consist of 8 points. Points 1-4 describe the left edge of grid (they are located in II quadrant).
		 * 					Points 5-8 describe right edge of grid (they are located in I quadrant).
		 * 					- 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of y coordinate;
		 * 					- 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge;
		 * 					- 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
		 * 					- 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
		 * 					- 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
		 * 					- 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
		 * 					- 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
		 *					- 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of y coordinate.
		 *			in		float smothing - smothing angle value
		 *
		 * @return 			-
		 *
		 * @remarks 		The function calculates masks for 3D BEV. The masks will be used for texture mapping.
		 * 					They must be defined for original captured image from camera (with fisheye distortion)
		 * 					because the same transformation will be applied on camera frames and masks.
		 *
		 * 					The procedure of mask calculation:
		 *					-	Calculate seams for every two adjacent grids. The seam of two adjacent grids is a line y = a * x + b.
		 *						Coefficients a and b have been found from grid intersection. The seam points have been defined for
		 *						3D template and then have been projected to an image plane using projecPoints function from OpenCV
		 *						library. To use the projectPoints function it is necessary to know extrinsic and intrinsic camera
		 *						parameters: camera matrix, distortion coefficients, rotation and translation vectors.
		 *					-	Create masks which are limited to seams. All 2D seam points describe a convex polygon which defines
		 *						the mask in 2D image. The polygon is filled with white color, and background is filled with black color.
		 *					-	Smooth mask edges.
		 *
		 **************************************************************************************************************/
		void createMasks(vector<Camera*> &cameras, vector< vector<Point3f> > &seam_points, float smothing);

		/**************************************************************************************************************
		 *
		 * @brief  			Split vertices/texels grid into grid which will be rendered with blending and grid which will
		 * 					be rendered without blending for all cameras
		 *
		 * @param  			-
		 *
		 * @return 			Functions returns 0 if all grid files "arrayX" file (X = 1,2,3,4 is camera number) are existed.
		 * 					If one of the files not found the function returns -1.
		 *
		 * @remarks 		The function reads grid of texels/vertices from the "arrayX" file (X = 1,2,3,4 is camera number)
		 * 					and produces two output grids: first for overlapping regions ("arrayX1") and second for
		 * 					non-overlapping regions ("arrayX2").
		 * 					Camera blending mask is used to split grid into two parts. The mask value has been checked
		 * 					for each texels of a rendered triangle. If at least one of texels is masked with value less than
		 * 					255 then the triangle is written to overlap grid. Otherwise the triangle is written to non-overlap
		 * 					grid.
		 *
		 **************************************************************************************************************/
		int splitGrids(void);

	private:		
		vector<Mat> masks;	// Vector of masks
		vector<Seam> seaml;	// Left seam
		vector<Seam> seamr; // Right seam

		/**************************************************************************************************************
		 *
		 * @brief  			Smooth mask
		 *
		 * @param  	in/out	Mat &img - mask
		 * 			in		Vec2b colors - 	range of colors [left edge color, right edge color]
		 * 									For left mask edge the color range will be [0, 255] - from black to white.
		 * 									And for right mask edge the color range will be [255, 0] - from white to black.
		 * 			in		Vec2d angles - 	smoothing will be apply in these angles in the left and right direction from mask border.
		 * 			in		double angle_step - step of color gradient
		 * 			in		vector<Point3f> edge_points - 	vector of template points. It must include 3 points:
		 * 													1. The first point of seam (intersection of bottom grid borders, z = 0)
		 * 													2. The intersection point of seam line and base circle (z = 0)
		 * 													3. The 3rd point of seam points (with maximum value of z coordinate)
		 * 			in		Camera* camera  - Camera object
		 *
		 * @return 			-
		 *
		 * @remarks 		The function smoothes the mask left and right edges to obtain a seamless blending of frames. The angle of
		 * 					smoothing (2 * SMOTHING_ANGLE) defines the area in which mask edge will be smooth. The original seam divides
		 * 					the smoothing angle into two angles with equal measures (angle bisector). The angle based smoothing is applied
		 * 					only for flat base. The seam at the bowl side is smoothed with constant width of smoothing.
		 *
		 **************************************************************************************************************/
		void smoothMaskEdge(Mat &img, Vec2b colors, Vec2d angles, double angle_step, vector<Point3f> edge_points, Camera* camera);

		/**************************************************************************************************************
		 *
		 * @brief  			Get intersection of two polygons
		 *
		 * @param  	in		vector<Point3f> &polygon1 - first convex polygon points. The polygon is described with 8 points:
		 * 					1st point - left bottom vertex, 4th point - left top vertex, 5th point - right top vertex,
		 * 					8th point - right bottom point. The polygon must be convex.
		 * 							 4 ____ 5
		 *		 					3 /    \ 6
		 * 							 |      |
		 * 							2 \____/ 7
		 *		 					 1      8
		 * 			in		vector<Point3f> &polygon2 - second convex polygon points. The polygon is described with 8 points:
		 * 					1st point - left bottom vertex, 4th point - left top vertex, 5th point - right top vertex,
		 * 					8th point - right bottom point. The polygon must be convex.
		 * 			out		Seam &seam - polygons intersection (2 points + line that goes through them)
		 * 			in		int rotation - direction of rotation for the second polygon. If rotation < 0, then the second polygon
		 * 					is rotated on 90 degrees angle to the left. Otherwise it is rotated on 90 degrees angle to the right.
		 *
		 * @return 			-
		 *
		 * @remarks 		The function search for two convex polygons intersection. The first intersection point is searched
		 * 					between polygons bottom sides 1-2, 1-8, 7-8. And the second intersection point is searched between
		 * 					top sides 4-5. The polygon intersection is calculated after second polygon points have been rotated
		 * 					according to the rotation value.
		 *
		 **************************************************************************************************************/
		void getSeamPoints(vector<Point3f> &polygon1, vector<Point3f> &polygon2, Seam &seam, int rotation);

};


#endif /* SRC_MASKS_HPP_ */
