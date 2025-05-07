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


#ifndef SRC_GRID_HPP_
#define SRC_GRID_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "camera.hpp"
#include "macros.hpp"

using namespace cv;
using namespace std;

/*******************************************************************************************
 * Types
 *******************************************************************************************/
struct GridParam {		/* Parameters of grid */
	uint angles;		/* Every quadrant of circle will be divided into this number of arcs */
	uint start_angle;	/* The parameters sets a circle segment for which the grid will be generated.
	 	 	 	 	 	 * It defines 2 points of circle secant. The first point is located in
	 	 	 	 	 	 * I quadrant and it is the start point of start_angle arc. And the second point is
	 	 	 	 	 	 * located in II quadrant and it is the end point of (angles - start_angle) arc */
	uint nop_z;			/* Number of points in z axis */
	double step_x;		/* Step in x axis which is used to define grid points in z axis.
	 	 	 	 	 	 * Step in z axis: step_z[i] = (i * step_x)^2, i = 1, 2, ... - number of point */
};

struct CameraInfo {		
	int width;
	int height;
	int index;
};

/*******************************************************************************************
 * Global functions
 *******************************************************************************************/
/**************************************************************************************************************
 *
 * @brief  			Rotate point according to the camera index
 *
 * @param  in		index - camera index
 * 		   in		Point3f point - grid point
 *
 * @return 			Point3f point - grid point after rotation
 *
 * @remarks 		The function rotate grid point according to the camera index value:
 * 						index = 0 - without rotation;
 * 						index = 1 - 90 degree clockwise rotation;
 * 						index = 2 - 180 degree clockwise rotation;
 * 						index = 3 - 270 degree clockwise rotation;
 *
 **************************************************************************************************************/
extern Point3f rotatePoint(int index, Point3f point);

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* CurvilinearGrid class - the grid is denser at the middle of bowl bottom and more sparse at the bowl bottom edge. */
class CurvilinearGrid {
	public:
	
	
	
	
	void setAngles(uint val) {parameters.angles = val; }
	void setStartAngle(uint val) {parameters.start_angle = val; }
	void setNopZ(uint val) {parameters.nop_z = val; }
	void setStepX(double val) {parameters.step_x = val; }
	
	
		/**************************************************************************************************************
		 *
		 * @brief  			CurvilinearGrid class constructor.
		 *
		 * @param  in 		uint angles 	  -	every quadrant of circle will be divided into this number of arcs.
		 * 					uint start_angle  -	It is not necessary to define grid through a whole semi-circle.
		 * 										The start_angle sets a circular sector for which the grid will be generated.
		 * 					uint nop_z 		  -	number of points in z axis.
		 * 					double step_x	  -	grid step in polar coordinate system. The value is also used to define
		 * 										grid points in z axis.
		 * 										Step in z axis: step_z[i] = (i * step_x)^2, i = 1, 2, ... - number of point.
		 *
		 * @return 			The function create the CurvilinearGrid object.
		 *
		 * @remarks 		The function sets main property of new CurvilinearGrid object.
		 *
		 **************************************************************************************************************/
		CurvilinearGrid(uint angles, uint start_angle, uint nop_z, double step_x);

		/**************************************************************************************************************
		 *
		 * @brief  			Copy points from seam property into the S vector.
		 *
		 * @param  out		vector<Point3f> &S - pointer to the output vector of seam points
		 *
		 * @return 			-
		 *
		 * @remarks 		The function copies points from seam property into the input vector S. If the seam_points
		 * 					vector size is not equal 8, then there was a problem with points searching and result of
		 * 					function is inapplicable.
		 *
		 **************************************************************************************************************/
		void getSeamPoints(vector<Point3f> &S) {S.clear(); for(uint i = 0; i < seam.size(); i ++) { S.push_back(seam[i]); }}

		/**************************************************************************************************************
		 *
		 * @brief  			Generate 3D grid of texels/vertices for the input Camera object.
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 * 		   in		double radius - radius of base circle. The radius must be defined relative to template width.
		 * 		   			The template width (in pixels) is considered as 1.0.
		 *
		 * @return 			-
		 *
		 * @remarks 		The function defines 3D grid, generates triangles from it and saves the triangles into file.
		 *
		 **************************************************************************************************************/
		void createGrid(Camera *camera, double radius);

	
		int getGrid(float** points);
	
			/**************************************************************************************************************
		 *
		 * @brief  			Generate triangles from a 3D grid and save them to a file.
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 *
		 * @return 			-
		 *
		 * @remarks 		The file with triangles description has been saved to the file arrayX, where X is camera index.
		 * 					The grid has been rotated according to the camera index value:
		 * 						index = 0 - without rotation;
		 * 						index = 1 - 90 degree clockwise rotation;
		 * 						index = 2 - 180 degree clockwise rotation;
		 * 						index = 3 - 270 degree clockwise rotation;
		 *
		 **************************************************************************************************************/
	void saveGrid(Camera* camera);
	private:
		CameraInfo cam_info;
	
		int NoP;				// Number of grid points for one grid sector (angle)
		vector<Point3f> p3d;	// 3D grid points (template points)
		vector<Point2f> p2d;	// 2D grid points (image points)
		GridParam parameters;	// Parameters of grid
		vector<Point3f> seam;	// Seam points

		/**************************************************************************************************************
		 *
		 * @brief  			Reorganize grid to clean middle part of view
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 * 		   in		double radius - radius of base circle. The radius must be defined relative to template width.
		 * 		   			The template width (in pixels) is considered as 1.0.
		 *
		 * @return 			-
		 *
		 * @remarks 		The function reorganize input 3d and 2d grids to clean middle part of view. It replace
		 * 					all point which are defined on 3d grid but aren't defined on camera frame with border values.
		 *
		 **************************************************************************************************************/
		void reorgGrid(double radius, Camera* camera);

		/**************************************************************************************************************
		 *
		 * @brief  			Find seam points.
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 * 		   out		vector<Point3f> &seam_points - pointer to the output vector containing seam points
		 *
		 * @return 			-
		 *
		 * @remarks			The function search 8 points to define the edge of grid. If the seam_points vector size is
		 * 					not equal 8, then there was a problem with points searching and result of function is
		 * 					inapplicable.
		 * 					Points 1-4 describe the left edge of grid (they are located in II quadrant).
		 * 					- 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of y coordinate;
		 * 					- 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge;
		 * 					- 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
		 * 					- 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
		 *
		 * 					Points 5-8 describe right edge of grid (they are located in I quadrant).
		 * 					- 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
		 * 					- 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
		 * 					- 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
		 *					- 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of y coordinate.
		 *
		 **************************************************************************************************************/
		void findSeam(Camera* camera, vector<Point3f> &seam_points);


};

/*  RectilinearGrid class - the grid is sparse at the middle of bowl bottom and more denser at the bowl bottom edge.
 *  To fill circle base with vertices grid the uneven grid is used. The circle is divided into arcs of same values
 *  and the intersection points of arcs define X and Y coordinates of grid corners. In this case the grid at the
 *  edge of circle is denser than at the center. */
class RectilinearGrid {
	public:

		void setAngles(uint val) {parameters.angles = val; }
		void setStartAngle(uint val) {parameters.start_angle = val; }
		void setNopZ(uint val) {parameters.nop_z = val; }
		void setStepX(double val) {parameters.step_x = val; }
	
	
		/**************************************************************************************************************
		 *
		 * @brief  			RectilinearGrid class constructor.
		 *
		 * @param  in 		uint angles 	  -	every quadrant of circle will be divided into this number of arcs.
		 * 					uint start_angle  -	It is not necessary to define grid through a whole semi-circle.
		 * 										The start_angle sets a circle segment for which the grid will be generated.
		 * 										The parameter defines 2 points of circle secant. The first point is located in
		 * 										I quadrant and it is the start point of start_angle arc. And the second point is
		 * 										located in II quadrant and it is the end point of (angles - start_angle) arc.
		 * 					uint nop_z 		  -	number of points in z axis.
		 * 					double step_x	  -	step in x axis which is used to define grid points in z axis.
		 * 										Step in z axis: step_z[i] = (i * step_x)^2, i = 1, 2, ... - number of point.
		 *
		 * @return 			The function create the RectilinearGrid object.
		 *
		 * @remarks 		The function sets main property of new RectilinearGrid object.
		 *
		 **************************************************************************************************************/
		RectilinearGrid(uint angles, uint start_angle, uint nop_z, double step_x);

		/**************************************************************************************************************
		 *
		 * @brief  			Copy points from seam property into the S vector.
		 *
		 * @param  out		vector<Point3f> &S - pointer to the output vector of seam points
		 *
		 * @return 			-
		 *
		 * @remarks 		The function copies points from seam property into the input vector S. If the seam_points
		 * 					vector size is not equal 8, then there was a problem with points searching and result of
		 * 					function is inapplicable.
		 *
		 **************************************************************************************************************/
		void getSeamPoints(vector<Point3f> &S) {S.clear(); for(uint i = 0; i < seam.size(); i ++) { S.push_back(seam[i]); } }

		/**************************************************************************************************************
		 *
		 * @brief  			Generate 3D grid of texels/vertices for the input Camera object.
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 * 		   in		double radius - radius of base circle. The radius must be defined relative to template width.
		 * 		   			The template width (in pixels) is considered as 1.0.
		 *
		 * @return 			-
		 *
		 * @remarks 		The function defines 3D grid, generates triangles from it and saves the triangles into file.
		 *
		 **************************************************************************************************************/
		void createGrid(Camera *camera, double radius);
	
	
	int getGrid(float** points);
	
	
			/**************************************************************************************************************
		 *
		 * @brief  			Generate triangles from a 3D grid and save them to a file.
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 *
		 * @return 			-
		 *
		 * @remarks 		The file with triangles description has been saved to the file arrayX, where X is camera index.
		 * 					The grid has been rotated according to the camera index value:
		 * 						index = 0 - without rotation;
		 * 						index = 1 - 90 degree clockwise rotation;
		 * 						index = 2 - 180 degree clockwise rotation;
		 * 						index = 3 - 270 degree clockwise rotation;
		 *
		 **************************************************************************************************************/
	void saveGrid(Camera* camera);
	
	
	private:
		CameraInfo cam_info;
	
		vector<int> NoP;	// Number of points in grid column
		vector<Point3f> p3d;	// 3D grid points (template points)
		vector<Point2f> p2d;	// 2D grid points (image points)
		GridParam parameters; // Parameters of grid
		vector<Point3f> seam; // Seam points

		/**************************************************************************************************************
		 *
		 * @brief  			Find seam points.
		 *
		 * @param  in		Camera* camera - pointer to the Camera object
		 * 		   out		vector<Point3f> &seam_points - pointer to the output vector containing seam points
		 *
		 * @return 			-
		 *
		 * @remarks			The function search 8 points to define the edge of grid. If the seam_points vector size is
		 * 					not equal 8, then there was a problem with points searching and result of function is
		 * 					inapplicable.
		 * 					Points 1-4 describe the left edge of grid (they are located in II quadrant).
		 * 					- 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of y coordinate;
		 * 					- 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge;
		 * 					- 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
		 * 					- 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
		 *
		 * 					Points 5-8 describe right edge of grid (they are located in I quadrant).
		 * 					- 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
		 * 					- 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
		 * 					- 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
		 *					- 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of y coordinate;
		 *
		 **************************************************************************************************************/
		void findSeam(Camera* camera, vector<Point3f> &seam_points);


};


#endif /* SRC_GRID_HPP_ */
