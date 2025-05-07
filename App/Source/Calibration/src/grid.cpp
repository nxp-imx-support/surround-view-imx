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

#include "grid.hpp"

float lerp(float v0, float v1, float t)
{
	return (1.0f - t) * v0 + t * v1;
}

float getMapAt(Mat map, Point2f coord)
{
	Point ind((int)coord.x, (int)coord.y);
	Point2f t = coord - Point2f(ind.x, ind.y);

	float v0 = lerp(map.at<float>(ind), map.at<float>(ind + Point(1, 0)), t.x);
	float v1 = lerp(map.at<float>(ind + Point(0, 1)), map.at<float>(ind + Point(1, 1)), t.x);
	float v = lerp(v0, v1, t.y);

	return v;
}

/**************************************************************************************************************
 *
 * @brief  			CurvilinearGrid class constructor.
 *
 * @param  in 		uint angles 	  -	every half of circle will be divided into this number of arcs.
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
CurvilinearGrid::CurvilinearGrid(uint angles, uint start_angle, uint nop_z, double step_x)
{
	parameters.angles = angles;
	parameters.start_angle = start_angle;
	parameters.nop_z = nop_z;
	parameters.step_x = step_x;
	cam_info.height = 0;
	cam_info.width = 0;
	cam_info.index = -1;
	NoP = 0;
}

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
void CurvilinearGrid::createGrid(Camera *camera, double radius)
{
	cam_info.height = camera->xmap.rows;
	cam_info.width = camera->xmap.cols;
	cam_info.index = camera->index;
		
	p3d.clear();
	p2d.clear();
	
	if (((camera->getRvec()).empty()) ||((camera->getTvec()).empty()) || (radius <= 0.0))
	{
		cout << "Grid " << camera->index << " was not generated" << endl;
		return;
	}		
		
	int pnum = static_cast<int>(radius / parameters.step_x);	// Number of grid rows of flat bowl bottom
	int startpnum = static_cast<int>((float)camera->tmp.ref_points[0].y / (float)camera->tmp.ref_points[0].x / (float)parameters.step_x / 3.0f);	// The first row of grid
	NoP = 2 * ((pnum - startpnum) + (int)parameters.nop_z); // Number of grid points for one grid sector (angle)

	for(uint ang = parameters.start_angle + 1U; ang <= (uint)(parameters.angles - parameters.start_angle); ang++)
	{
		double angle_start = (double)((double)ang - 1.0) * (M_PI / (double)parameters.angles); // Start angle of the current circular sector
		double angle_end = (double)ang * (M_PI / (double)parameters.angles); // End angle of the current circular sector

		// Flat bottom points
		for(int i = startpnum; i < pnum; i++)
		{
			double hptn = (double)i * parameters.step_x;
			p3d.push_back(Point3f(hptn * cos(angle_end), - hptn * sin(angle_end), 0.0));
			p3d.push_back(Point3f(hptn * cos(angle_start), - hptn * sin(angle_start), 0.0));
		}

		// Points on bowl side
		for(uint i = 1; i <= parameters.nop_z; i++)
		{
			double hptn = radius + (double)i * parameters.step_x;
			p3d.push_back(Point3f(hptn * cos(angle_end), - hptn * sin(angle_end), - pow((double)i * parameters.step_x, 2)));
			p3d.push_back(Point3f(hptn * cos(angle_start), - hptn * sin(angle_start), - pow((double)i * parameters.step_x, 2)));
		}
	}

	
	// Projects 3D points to an image plane
	projectPoints(p3d, camera->getRvec(), camera->getTvec(), camera->getK(), camera->getDistCoeffs(), p2d);

	// Reorganize grid to clean middle part of view
	reorgGrid(radius, camera);

	// Find seam points
	findSeam(camera, seam);
}







int CurvilinearGrid::getGrid(float** points)
{
	int size = 0;
	
	if (p2d.size() == 0) { return (0); }
	
	Point2f top = Point2f((static_cast<float>(cam_info.index & 1U) - 1.0f), static_cast<float>((~cam_info.index >> 1U) & 1U));
		
	float x_norm = 1.0f / (float)cam_info.width;
	float y_norm = 1.0f / (float)cam_info.height;
		
	(*points) = (float*)malloc(6 * p2d.size() * sizeof(float));
	if ((*points) == NULL) {
		cout << "Memory allocation did not complete successfully" << endl;
		return(0);
	}

	for (uint i = 0; i < p2d.size(); i++)
	{
		if((p2d[i].x < (float)cam_info.width) && (p2d[i].y < (float)cam_info.height) &&
		   (p2d[i].x > 0.0) && (p2d[i].y > 0.0))
		{
			(*points)[size] = p2d[i].x * x_norm + top.x;
			(*points)[size + 1] = top.y - p2d[i].y * y_norm;
			(*points)[size + 2] = 0.0f;
			
			size += 3;
		}
	}

	return (size);
}

























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
void CurvilinearGrid::reorgGrid(double radius, Camera* camera)
{
	double grid_step = radius / 4.0;

	vector<Point3f> point_3d; // 3D coordinates of the closest point for the camera
	point_3d.push_back(Point3f(0.0, (float)(-grid_step), 0.0)); // Start value

	Mat distortion_mask(camera->xmap.rows, camera->xmap.cols, CV_8U, Scalar(255, 255, 255)); // Mask for defisheye image
	camera->defisheye(distortion_mask, distortion_mask);

	// Get closest point for the camera which exists on camera frame
	for(int i = 0; i < 10; i ++) // 10 iteration is enough
	{
		vector<Point2f> point_2d; // 2D coordinates of the closest point for the camera on defisheye image
		projectPoints(point_3d, camera->getRvec(), camera->getTvec(), camera->getK(), camera->getDistCoeffs(), point_2d);
		grid_step = grid_step / 2.0; // Increase step
		if( (point_2d[0].x < (float)distortion_mask.cols) && (point_2d[0].y < (float)distortion_mask.rows) &&
			(point_2d[0].x >= 0.0) && (point_2d[0].y >= 0.0))
		{
			if(distortion_mask.at<uchar>(point_2d[0]) == 0U) // Check mask value
			{
				point_3d[0].y -= grid_step; // Go down
			}
			else
			{
				point_3d[0].y += grid_step; // Go up
			}
		}
		else
		{
			point_3d[0].y -= grid_step; // Go down
		}
	}

	// Recalculate 3D and 2D grid points
	for(uint i = 0; i < p3d.size(); i ++) // Check all grid points
	{
		if(p3d[i].y > point_3d[0].y) // If grid point is lays in masking region
		{
			vector<Point2f> p2tmp;
			vector<Point3f> p3tmp;
			p3d[i].y = point_3d[0].y; // New value of 3D point
			p3tmp.push_back(p3d[i]);
			projectPoints(p3tmp, camera->getRvec(), camera->getTvec(), camera->getK(), camera->getDistCoeffs(), p2tmp);
			p2d[i] = p2tmp[0]; // New value of 2D point
		}
	}
}

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
void CurvilinearGrid::findSeam(Camera* camera, vector<Point3f> &seam_points)
{
	bool isfound = false;
	int middle_angle = ((int)parameters.angles - 2 * (int)parameters.start_angle) / 2; // index of the middle angle
	seam_points.clear(); // Clear output vector

	// Get mask for defisheye transformation
	Mat defisheye_mask(camera->xmap.rows, camera->xmap.cols, CV_8U, Scalar(255));
	remap(defisheye_mask, defisheye_mask, camera->xmap, camera->ymap, cv::INTER_LINEAR);


	/*********************************************************************************************************************
	 * p1 - 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of radial
	 * 		coordinate in polar coordinate system
	 *********************************************************************************************************************/
	int point_num = 1;
	while((!isfound) && (point_num < (NoP - 2 * (int)parameters.nop_z)))
	{
		for(int ang = (int)parameters.angles - 2 * (int)parameters.start_angle - 1; ang >= middle_angle; ang--)
		{
			int idx = ang * NoP + point_num;
			if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
			   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
			{
				if((defisheye_mask.data != NULL) && (defisheye_mask.at<uchar>((int)round(p2d[idx].y),(int)round(p2d[idx].x)) != 0U))
				{
					seam_points.push_back(p3d[idx]);
					isfound = true;
					break;
				}
			}
		}
		point_num += 2;
	}

	/*********************************************************************************************************************
	 * p2 - 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge
	 *********************************************************************************************************************/
	for(int ang = (int)parameters.angles - 2 * (int)parameters.start_angle; ang >= middle_angle; ang--)
	{
		int idx = NoP * ang - 2 * (int)parameters.nop_z - 2;
		if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
		   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
		{
			seam_points.push_back(p3d[idx]);
			break;
		}
	}

	/*************************************************************************************************************
	 * p3 - 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
	 *************************************************************************************************************/
	int angle_num = (int)parameters.angles - 2 * (int)parameters.start_angle;
	isfound = false;
	while((!isfound) && (angle_num >= middle_angle))
	{
		for(int idx = NoP * angle_num - 2; idx >= NoP * angle_num - 2 * (int)parameters.nop_z; idx-=2)
		{
			if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
			   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
			{
				seam_points.push_back(p3d[idx]);
				isfound = true;
				break;
			}
		}
		angle_num--;
	}

	/*********************************************************************************************************************
	 * p4 - 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
	 *********************************************************************************************************************/
	angle_num = (int)parameters.angles - 2 * (int)parameters.start_angle;
	isfound = false;
	while((!isfound) && (angle_num >= middle_angle))
	{
		int idx = NoP * angle_num - 2;
		if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
		   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
		{
			seam_points.push_back(p3d[idx]);
			isfound = true;
		}
		angle_num--;
	}


	/*********************************************************************************************************************
	 * p5 - 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
	 *********************************************************************************************************************/
	angle_num = 1;
	isfound = false;
	while((!isfound) && (angle_num <= (int)middle_angle))
	{
		int idx = NoP * angle_num - 1;
		if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
		   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
		{
			seam_points.push_back(p3d[idx]);
			isfound = true;
		}
		angle_num++;
	}

	/*************************************************************************************************************
	 * p6 - 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
	 *************************************************************************************************************/
	angle_num = 1;
	isfound = false;
	while((!isfound) && (angle_num <= (int)middle_angle))
	{
		for(int idx = NoP * angle_num - 1; idx >= NoP * angle_num - 2 * (int)parameters.nop_z; idx-=2)
		{
			if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
			   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
			{
				seam_points.push_back(p3d[idx]);
				isfound = true;
				break;
			}
		}
		angle_num++;
	}


	/*********************************************************************************************************************
	 * p7 - 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
	 *********************************************************************************************************************/
	for(int ang = 0; ang <= middle_angle; ang++)
	{
		int idx = NoP * (ang + 1) - 2 * (int)parameters.nop_z - 1;
		if(((int)round(p2d[idx].x) < camera->xmap.cols) && (p2d[idx].x >= 0.0) &&
		   ((int)round(p2d[idx].y) < camera->xmap.rows) && (p2d[idx].y >= 0.0))
		{
			seam_points.push_back(p3d[idx]);
			break;
		}
	}

	/*********************************************************************************************************************
	 * p8 - 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of radial
	 * 		coordinate in polar coordinate system
	*********************************************************************************************************************/
	point_num = 1;
	isfound = false;
	while((!isfound) && (point_num < NoP - 2 * (int)parameters.nop_z))
	{
		for(int ang = 0; ang < (int)middle_angle; ang++)
		{
			int idx = ang * NoP + point_num;
			if(((int)round(p2d[idx].x) < defisheye_mask.cols) && (p2d[idx].x >= 0.0) &&
			   ((int)round(p2d[idx].y) < defisheye_mask.rows) && (p2d[idx].y >= 0.0))
			{
				if((defisheye_mask.data != NULL) &&(defisheye_mask.at<uchar>((int)round(p2d[idx].y), (int)round(p2d[idx].x)) != 0U))
				{
					seam_points.push_back(p3d[idx]);
					isfound = true;
					break;
				}
			}
		}
		point_num += 2;
	}
}

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
void CurvilinearGrid::saveGrid(Camera* camera)
{
	float height = (float)camera->xmap.rows;	// 2D grid height (texels)
	float width = (float)camera->xmap.cols;	// 2D grid width (texels)

	// Generate output array
	char file_name[50];
	sprintf(file_name, "./array%d", camera->index + 1);

    ofstream outC; // Output file
    outC.open(file_name, std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.

    for(uint ang = 0U; ang < parameters.angles - 2U * parameters.start_angle; ang++)
	{
		for(int i = 0; i < NoP - 2; i+=2)
		{
			int p = (int)ang * NoP + i;

		    /**************************** Get triangles for I quadrant of template **********************************
		     *   							  p  _  p+2
		     *   Triangles orientation: 		| /|		1st triangle (p - p+1 - p+2)
		     *   								|/_|		2nd triangle (p+1 - p+2 - p+3)
		     *   							 p+1    p+3
		     *******************************************************************************************************/

			if((round(p2d[p + 1].x) < width) && (round(p2d[p + 1].y) < height) && (p2d[p + 1].x >= 0.0) && (p2d[p + 1].y >= 0.0) &&
			   (round(p2d[p + 2].x) < width) && (round(p2d[p + 2].y) < height) && (p2d[p + 2].x >= 0.0) && (p2d[p + 2].y >= 0.0))
			{
				// Recalculate point for fisheye image
				Point2f t2 = Point2f(getMapAt(camera->xmap, p2d[p + 1]) / width, getMapAt(camera->ymap, p2d[p + 1]) / height);
				Point2f t3 = Point2f(getMapAt(camera->xmap, p2d[p + 2]) / width, getMapAt(camera->ymap, p2d[p + 2]) / height);

				// Rotate grid point according to the template
				Point3f v2 = rotatePoint(camera->index, p3d[p + 1]);
				Point3f v3 = rotatePoint(camera->index, p3d[p + 2]);

				// 1st triangle (p - p+1 - p+2)
				if((round(p2d[p].x) < width) && (round(p2d[p].y) < height) && (p2d[p].x >= 0.0) && (p2d[p].y >= 0.0))
				{
					Point3f v1 = rotatePoint(camera->index, p3d[p]);
					Point2f t1 = Point2f(getMapAt(camera->xmap, p2d[p]) / width, getMapAt(camera->ymap, p2d[p]) / height);

					// Save triangle points to the output file
					outC << v1.x << " " << v1.y << " " << v1.z << " " << t1.x << " " << t1.y << endl;
					outC << v2.x << " " << v2.y << " " << v2.z << " " << t2.x << " " << t2.y << endl;
					outC << v3.x << " " << v3.y << " " << v3.z << " " << t3.x << " " << t3.y << endl;
				}

				// 2nd triangle (p+1 - p+2 - p+3)
				if((round(p2d[p + 3].x) < width) && (round(p2d[p + 3].y) < height) && (p2d[p + 3].x >= 0.0) && (p2d[p + 3].y >= 0.0))
				{
					Point3f v4 = rotatePoint(camera->index, p3d[p + 3]);
					Point2f t4 = Point2f(getMapAt(camera->xmap, p2d[p + 3]) / width, getMapAt(camera->ymap, p2d[p + 3]) / height);

					// Save triangle points to the output file
					outC << v2.x << " " << v2.y << " " << v2.z << " " << t2.x << " " << t2.y << endl;
					outC << v4.x << " " << v4.y << " " << v4.z << " " << t4.x << " " << t4.y << endl;
					outC << v3.x << " " << v3.y << " " << v3.z << " " << t3.x << " " << t3.y << endl;
				}
			}
		}
	}

	outC.close(); // Close file
}























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
RectilinearGrid::RectilinearGrid(uint angles, uint start_angle, uint nop_z, double step_x)
{
	parameters.angles = angles;
	parameters.start_angle = start_angle;
	parameters.nop_z = nop_z;
	parameters.step_x = step_x;
	cam_info.height = 0;
	cam_info.width = 0;
	cam_info.index = -1;
}

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
void RectilinearGrid::createGrid(Camera *camera, double radius)
{
	cam_info.height = camera->xmap.rows;
	cam_info.width = camera->xmap.cols;
	cam_info.index = camera->index;
	
	p3d.clear();
	p2d.clear();
	NoP.clear();
	
	if (((camera->getRvec()).empty()) || ((camera->getTvec()).empty()) || (radius <= 0.0))
	{
		cout << "Grid " << camera->index << " was not generated" << endl;
		return;
	}	
	
	double step_x_2 = parameters.step_x * parameters.step_x;

	for(uint i = 0U; i < 2U * (parameters.angles - parameters.start_angle); i++) {
		NoP.push_back(0);
	}

	// X and Y coordinates for circle base
	vector<double> xxx; // Array of x coordinates of grid (in row z = 0, y = 0)
	vector<double> yyy; // Array of y coordinates of grid (in column z = 0, x = 0)

	// The circle is divided into arcs of same values and the intersection points of arcs define X and Y coordinates of grid corners
	for(uint j = 0; j <= parameters.angles; j++)
	{
		double ang = (double)j * (M_PI_2 / (double)parameters.angles); // Arcs are defined by angles
		xxx.push_back(-radius * cos(ang));
		yyy.push_back(-radius * sin(ang));
	}

	// For each column of grid (II quadrant)
	for(uint j = parameters.start_angle; j < xxx.size(); j++)
	{
		// Add points of circle base (z = 0)
		for(uint k = parameters.start_angle; k <= j; k++) {
			p3d.push_back(Point3f(xxx[j], yyy[k], 0));
		}
		// Add 3D part of grid (bowl edge)
		for(double zz = 1.0; zz <= (double)parameters.nop_z; zz++)
		{
			double new_z = zz * zz * step_x_2;
			double new_x = xxx[j] * (1.0 + sqrt(new_z) / radius);
			double new_y = yyy[j] * (1.0 + sqrt(new_z) / radius);
			p3d.push_back(Point3f(new_x, new_y, -new_z));
		}
		// Set the number of grid point in the column
		NoP[j - parameters.start_angle] = (int)j - (int)parameters.start_angle + 1 + (int)parameters.nop_z;
	}

	// For each column of grid (I quadrant)
	for(int j = (int)xxx.size() - 2; j >= (int)parameters.start_angle; j--)
	{
		// Add points of circle base (z = 0)
		for(int k = (int)parameters.start_angle; k <= j; k++) {
			p3d.push_back(Point3f(- xxx[j], yyy[k], 0));
		}
		// Add 3D part of grid (bowl edge)
		for(double zz = 1.0; zz <= (double)parameters.nop_z; zz++)
		{
			double new_z = zz * zz * step_x_2;
			double new_x = xxx[j] * (-1.0 - sqrt(new_z) / radius);
			double new_y = yyy[j] * (1.0 + sqrt(new_z) / radius);
			p3d.push_back(Point3f(new_x, new_y, -new_z));
		}
		// Set the number of grid point in the column
		NoP[2 * (int)xxx.size() - j - (int)parameters.start_angle - 2] = j - (int)parameters.start_angle + 1 + (int)parameters.nop_z;
	}

	// Projects 3D points to an image plane
	projectPoints(p3d, camera->getRvec(), camera->getTvec(), camera->getK(), camera->getDistCoeffs(), p2d);

	// Find seam points
	findSeam(camera, seam);

}







int RectilinearGrid::getGrid(float** points)
{
	int size = 0;
	
	if (p2d.size() == 0) { return (0); }
		
	Point2f top = Point2f((static_cast<float>(cam_info.index & 1U) - 1.0f), static_cast<float>((~cam_info.index >> 1U) & 1U));
		
	float x_norm = 1.0f / (float)cam_info.width;
	float y_norm = 1.0f / (float)cam_info.height;
		
	(*points) = (float*)malloc(6 * p2d.size() * sizeof(float));
	if ((*points) == NULL) {
		cout << "Memory allocation did not complete successfully" << endl;
		return(0);
	}

	for (uint i = 0; i < p2d.size(); i++)
	{
		if ((p2d[i].x < (float)cam_info.width) && (p2d[i].y < (float)cam_info.height) &&
		   (p2d[i].x > 0.0) && (p2d[i].y > 0.0))
		{
			(*points)[size] = p2d[i].x * x_norm + top.x;
			(*points)[size + 1] = top.y - p2d[i].y * y_norm;
			(*points)[size + 2] = 0.0f;
			
			size += 3;
		}
	}

	return (size);
}














/**************************************************************************************************************
 *
 * @brief  			Find seam points.
 *
 * @param  in		Camera* camera - pointer to the Camera object
 * 		   			(vector size is equal the number of grid column)
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
void RectilinearGrid::findSeam(Camera* camera, vector<Point3f> &seam_points)
{
	Point3f p;
	int width = camera->xmap.cols; // Grid width
	int height = camera->xmap.rows; // Grid height
	int offset = 0;
	double min_y = - 100.0;
	double min_z = 0.0;

	int point_num = 0; // Number of grid points
	for(uint i = 0; i < NoP.size(); i++) // for all column of grid
	{
		point_num += NoP[i]; // Add number of point in i column
	}

	seam_points.clear(); // Clear output vector

	/*********************************************************************************************************************
	 * p1 - 1st point is located on flat circle base (z = 0). It is the leftmost point with minimum value of y coordinate
	 *********************************************************************************************************************/
	for(uint i = 0; i < NoP.size() / 2; i++) // For all column in II quadrant
	{
		for(uint j = 0; j < (NoP[i] - parameters.nop_z); j++) // Check only flat base circle
		{
			if(((int)p2d[offset + (int)j].x < width) && ((int)p2d[offset + (int)j].y < height) && (p2d[offset + (int)j].x >= 0.0) && (p2d[offset + (int)j].y >= 0.0)) // Check if point belongs to grid
			{
				if(min_y < p3d[offset + (int)j].y)
				{
					min_y = p3d[offset + (int)j].y; // Get minimum y
					p = p3d[offset + (int)j];
				}
			}
		}
		offset += NoP[i]; // Add number of point in i column
	}
	seam_points.push_back(p); // Push p1 to the output vector

	/*********************************************************************************************************************
	 * p2 - 2nd point is located on flat circle base (z = 0). It is the leftmost point of grid which lies on base circle edge
	 *********************************************************************************************************************/
	offset = 0; // Reset offset value
	for(uint i = 0; i < NoP.size() / 2; i++) // For all column in II quadrant
	{
		int edge_index = offset + NoP[i] - (int)parameters.nop_z - 1; // Get index of point which lies on base circle edge in i column
		if(((int)p2d[edge_index].x < width) && ((int)p2d[edge_index].y < height) && (p2d[edge_index].x >= 0.0) && (p2d[edge_index].y >= 0.0)) // Check if point belongs to grid
		{
			seam_points.push_back(p3d[edge_index]); // Push p2 to the output vector

			/*************************************************************************************************************
			 * p3 - 3rd point is located on bowl edge. It is the last point in first grid column with (z != 0);
			 *************************************************************************************************************/
			for(int k = edge_index + 1; k < offset + NoP[i]; k++)
			{
				if(((int)p2d[k].x > width) || ((int)p2d[k].y > height) || (p2d[k].x < 0.0) || (p2d[k].y < 0.0)) // Check if point does not belong to grid
				{
					seam_points.push_back(p3d[k - 1]); // Push p3 to the output vector
					break;
				}
				else {
					if(k == offset + NoP[i] - 1) {// Check if k point is the last in the i column
						seam_points.push_back(p3d[k]); // Push p3 to the output vector
					}
				}
			}
			break;
		}
		offset += NoP[i]; // Add number of point in i column
	}

	/*********************************************************************************************************************
	 * p4 - 4th point is located on bowl edge. It is the leftmost point with maximum value of z coordinate.
	 *********************************************************************************************************************/
	offset = 0; // Reset offset value
	for(uint i = 0; i < NoP.size() / 2; i++) // For all column in II quadrant
	{
		for(int j = NoP[i] - (int)parameters.nop_z; j < NoP[i]; j++) // Check only bowl edge with z != 0
		{
			if(((int)p2d[offset + j].x < width) && ((int)p2d[offset + j].y < height) && (p2d[offset + j].x >= 0.0) && (p2d[offset + j].y >= 0.0)) // Check if point belongs to grid
			{
				if(min_z > p3d[offset + j].z)
				{
					min_z = p3d[offset + j].z; // Get minimum z
					p = p3d[offset + j];
				}
			}
		}
		offset += NoP[i]; // Add number of point in i column
	}
	seam_points.push_back(p); // Push p4 to the output vector

	/*********************************************************************************************************************
	 * p5 - 5th point is located on bowl edge. It is the rightmost point with maximum value of z coordinate.
	 *********************************************************************************************************************/
	offset = point_num; // Set offset
	min_z = 0.0; // Reset minimum z
	for(uint i = (uint)NoP.size() - 1U; i > (uint)NoP.size() / 2U; i--) // For all column in I quadrant
	{
		for(uint j = 0; j < parameters.nop_z; j++) // Check only bowl edge with z != 0
		{
			if(((int)p2d[offset - (int)j].x < width) && ((int)p2d[offset - (int)j].y < height) && (p2d[offset - (int)j].x >= 0.0) && (p2d[offset - (int)j].y >= 0.0)) // Check if point belongs to grid
			{
				if(min_z > p3d[offset - (int)j].z)
				{
					min_z = p3d[offset - (int)j].z; // Get minimum z
					p = p3d[offset - (int)j];
				}
			}
		}
		offset -= NoP[i]; // Subtract number of point in i column
	}
	seam_points.push_back(p); // Push p5 to the output vector

	/*********************************************************************************************************************
	 * p7 - 7th point is located on flat circle base (z = 0). It is the rightmost point of grid which lies on base circle edge;
	 *********************************************************************************************************************/
	offset = point_num; // Set offset
	for(uint i = (uint)NoP.size() - 1U; i > (uint)NoP.size() / 2U; i--) // For all column in I quadrant
	{
		int edge_index = offset - (int)parameters.nop_z - 1; // Get index of point which lies on base circle edge in i column
		if(((int)p2d[edge_index].x < width) && ((int)p2d[edge_index].y < height) && (p2d[edge_index].x >= 0.0) && (p2d[edge_index].y >= 0.0)) // Check if point belongs to grid
		{
			/*************************************************************************************************************
			 * p6 - 6th point is located on bowl edge. It is the last point in last grid column with (z != 0);
			 * 	 *********************************************************************************************************/
			for(int k = edge_index + 1; k < offset; k++)
			{
				if(((int)p2d[k].x > width) || ((int)p2d[k].y > height) || (p2d[k].x < 0.0) || (p2d[k].y < 0.0)) // Check if point does not belong to grid
				{
					seam_points.push_back(p3d[k - 1]); // Push p6 to the output vector
					seam_points.push_back(p3d[edge_index]); // Push p7 to the output vector
					break;
				}
				else {
					if(k == offset - 1) { // Check if k point is the last in the i column
						seam_points.push_back(p3d[k]); // Push p6 to the output vector
						seam_points.push_back(p3d[edge_index]); // Push p7 to the output vector
					}
				}
			}
			break;
		}
		offset -= NoP[i]; // Subtract number of point in i column
	}

	/*********************************************************************************************************************
	 * p8 - 8th point is located on flat circle base (z = 0). It is the rightmost point with minimum value of y coordinate;
	 *********************************************************************************************************************/
	offset = point_num; // Set offset
	min_y = -100.0; // Reset minimum y
	for(int i = (int)NoP.size() / 2 - 1; i > 0; i--) // For all column in I quadrant
	{
		for(int j = NoP[i] - (int)parameters.nop_z - 1; j > 0; j--) // Check only flat base circle
		{
			if(((int)p2d[offset - j].x < width) && ((int)p2d[offset - j].y < height) && (p2d[offset - j].x >= 0.0) && (p2d[offset - j].y >= 0.0)) // Check if point belongs to grid
			{
				if((min_y < p3d[offset - j].y) && (p3d[offset - j].z == 0.0))
				{
					min_y = p3d[offset - j].y; // Get minimum y
					p = p3d[offset - j];
				}
			}
		}
		offset -= NoP[i]; // Subtract number of point in i column
	}
	seam_points.push_back(p); // Push p8 to the output vector
}




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
void RectilinearGrid::saveGrid(Camera* camera)
{
	float height = (float)camera->xmap.rows;	// 2D grid height (texels)
	float width = (float)camera->xmap.cols;	// 2D grid width (texels)

	// Generate output array
	char file_name[50];
	sprintf(file_name, "./array%d", camera->index + 1);

    ofstream outC; // Output file
    outC.open(file_name, std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.

    int offset = NoP[0]; // Set offset of point in 3D grid (vertices)

    /**************************** Get triangles for II quadrant of template *********************************
     *   							  p3 _  p2
     *   Triangles orientation: 		| /|		1 triangle (p4-p1-p2)
     *   								|/_|		2 triangle (p4-p2-p3)
     *   							  p4   p1
     *******************************************************************************************************/

	for(uint xx = 1U; xx <= parameters.angles - parameters.start_angle; xx++) // Repeat for each column of 3D grid
	{
		for(int yy = 0; yy < NoP[xx]; yy ++) // Repeat for each row of the xx column
		{
			int p1 = offset + yy;						// Point with current offset (x, y)
			int p2 = offset + (yy + 1);					// Next point in the same column (x, y + 1)
			int p3 = offset + (yy + 1) - NoP[xx - 1U];	// Neighbor point from previous column (x - 1, y + 1)
			int p4 = offset + yy - NoP[xx - 1U];			// Neighbor point from previous column (x - 1, y)

			if((p4 < offset) && (p2 < offset + NoP[xx])) // Check if points p2 and p4 are exist in the 3D vertices grid
			{
			    /*******************************************************************************************************
			     *   							  		p2
			     *   1 triangle (p4-p1-p2): 		  /|
			     *   								 /_|
			     *   							  p4   p1
			     *******************************************************************************************************/
				if((round(p2d[p2].x) < width) && (round(p2d[p2].y) < height) && (p2d[p2].x >= 0.0) && (p2d[p2].y >= 0.0) &&
				   (round(p2d[p4].x) < width) && (round(p2d[p4].y) < height) && (p2d[p4].x >= 0.0) && (p2d[p4].y >= 0.0))
				{
					if (((int)round(p2d[p1].x) < camera->xmap.cols) && ((int)round(p2d[p1].y) < camera->ymap.rows) && (p2d[p1].x >= 0.0) && (p2d[p1].y >= 0.0))
					{
						// Recalculate point for fisheye image
						Point2f p4_new = Point2f(getMapAt(camera->xmap, p2d[p4]) / width, getMapAt(camera->ymap, p2d[p4]) / height);
						Point2f p1_new = Point2f(getMapAt(camera->xmap, p2d[p1]) / width, getMapAt(camera->ymap, p2d[p1]) / height);
						Point2f p2_new = Point2f(getMapAt(camera->xmap, p2d[p2]) / width, getMapAt(camera->ymap, p2d[p2]) / height);

						// Rotate grid point according to the template
						Point3f vertex4 = rotatePoint(camera->index, p3d[p4]);
						Point3f vertex1 = rotatePoint(camera->index, p3d[p1]);
						Point3f vertex2 = rotatePoint(camera->index, p3d[p2]);

						// Save triangle points to the output file
						outC << vertex4.x << " " << vertex4.y << " " << vertex4.z << " " << p4_new.x << " " << p4_new.y << endl;
						outC << vertex1.x << " " << vertex1.y << " " << vertex1.z << " " << p1_new.x << " " << p1_new.y << endl;
						outC << vertex2.x << " " << vertex2.y << " " << vertex2.z << " " << p2_new.x << " " << p2_new.y << endl;
					}

					/*******************************************************************************************************
				     *   							  p3 _	p2
				     *   2 triangle (p4-p2-p3): 		| /
				     *   								|/
				     *   							  p4
				     *******************************************************************************************************/
					if((p3 < offset) && (round(p2d[p3].x) < width) && (round(p2d[p3].y) < height) && (p2d[p3].x >= 0.0) && (p2d[p3].y >= 0.0))
					{
						// Recalculate point for fisheye image
						Point2f p4_new = Point2f(getMapAt(camera->xmap, p2d[p4]) / width, getMapAt(camera->ymap, p2d[p4]) / height);
						Point2f p2_new = Point2f(getMapAt(camera->xmap, p2d[p2]) / width, getMapAt(camera->ymap, p2d[p2]) / height);
						Point2f p3_new = Point2f(getMapAt(camera->xmap, p2d[p3]) / width, getMapAt(camera->ymap, p2d[p3]) / height);

						// Rotate grid point according to the template
						Point3f vertex4 = rotatePoint(camera->index, p3d[p4]);
						Point3f vertex2 = rotatePoint(camera->index, p3d[p2]);
						Point3f vertex3 = rotatePoint(camera->index, p3d[p3]);

						// Save triangle points to the output file
						outC << vertex4.x << " " << vertex4.y << " " << vertex4.z << " " << p4_new.x << " " << p4_new.y << endl;
						outC << vertex2.x << " " << vertex2.y << " " << vertex2.z << " " << p2_new.x << " " << p2_new.y << endl;
						outC << vertex3.x << " " << vertex3.y << " " << vertex3.z << " " << p3_new.x << " " << p3_new.y << endl;
					 }
				}
			}
		}
		offset += NoP[xx]; // Update offset
	}

    /**************************** Get triangles for I quadrant of template **********************************
     *   							  p3 _  p2
     *   Triangles orientation: 		|\ |		1 triangle (p4-p1-p3)
     *   								|_\|		2 triangle (p1-p2-p3)
     *   							  p4   p1
     *******************************************************************************************************/
	for(uint xx = parameters.angles + 1U - parameters.start_angle; xx < 2U * (parameters.angles - parameters.start_angle) + 1U; xx++)
	{
		for(int yy = 0; yy < NoP[xx]; yy ++)
		{
			int p1 = offset + yy;						// Point with current offset (x, y)
			int p2 = offset + (yy + 1);					// Next point in the same column (x, y + 1)
			int p3 = offset + (yy + 1) - NoP[xx - 1U];	// Neighbor point from previous column (x - 1, y + 1)
			int p4 = offset + yy - NoP[xx - 1U];			// Neighbor point from previous column (x - 1, y)

			if(p3 < offset) // Check if point p3 is exist in the 3D vertices grid
			{
			    /*******************************************************************************************************
			     *   							  p3
			     *   1 triangle (p4-p1-p3): 		 |\
			     *   								 |_\
			     *   							  p4   p1
			     *******************************************************************************************************/
				if((round(p2d[p1].x) < width) && (round(p2d[p1].y) < height) && (p2d[p1].x >= 0.0) && (p2d[p1].y >= 0.0) &&
				   (round(p2d[p3].x) < width) && (round(p2d[p3].y) < height) && (p2d[p3].x >= 0.0) && (p2d[p3].y >= 0.0))
				{
					if ((round(p2d[p4].x) < width) && (round(p2d[p4].y) < height) && (p2d[p4].x >= 0.0) && (p2d[p4].y >= 0.0))
					{
						// Recalculate point for fisheye image
						Point2f p4_new = Point2f(getMapAt(camera->xmap, p2d[p4]) / width, getMapAt(camera->ymap, p2d[p4]) / height);
						Point2f p1_new = Point2f(getMapAt(camera->xmap, p2d[p1]) / width, getMapAt(camera->ymap, p2d[p1]) / height);
						Point2f p3_new = Point2f(getMapAt(camera->xmap, p2d[p3]) / width, getMapAt(camera->ymap, p2d[p3]) / height);

						// Rotate grid point according to the template
						Point3f vertex4 = rotatePoint(camera->index, p3d[p4]);
						Point3f vertex1 = rotatePoint(camera->index, p3d[p1]);
						Point3f vertex3 = rotatePoint(camera->index, p3d[p3]);

						// Save triangle points to the output file
						outC << vertex4.x << " " << vertex4.y << " " << vertex4.z << " " << p4_new.x << " " << p4_new.y << endl;
						outC << vertex1.x << " " << vertex1.y << " " << vertex1.z << " " << p1_new.x << " " << p1_new.y << endl;
						outC << vertex3.x << " " << vertex3.y << " " << vertex3.z << " " << p3_new.x << " " << p3_new.y << endl;
					}

					/*******************************************************************************************************
				     *   							  p3  _	p2
				     *   2 triangle (p1-p2-p3): 		 \ |
				     *   								  \|
				     *   							  		p1
				     *******************************************************************************************************/
					if((p2 < offset + NoP[xx]) && (round(p2d[p2].x) < width) && (round(p2d[p2].y) < height) && (p2d[p2].x >= 0.0) && (p2d[p2].y >= 0.0))
					{
						// Recalculate point for fisheye image
						Point2f p1_new = Point2f(getMapAt(camera->xmap, p2d[p1]) / width, getMapAt(camera->ymap, p2d[p1]) / height);
						Point2f p2_new = Point2f(getMapAt(camera->xmap, p2d[p2]) / width, getMapAt(camera->ymap, p2d[p2]) / height);
						Point2f p3_new = Point2f(getMapAt(camera->xmap, p2d[p3]) / width, getMapAt(camera->ymap, p2d[p3]) / height);

						// Rotate grid point according to the template
						Point3f vertex1 = rotatePoint(camera->index, p3d[p1]);
						Point3f vertex2 = rotatePoint(camera->index, p3d[p2]);
						Point3f vertex3 = rotatePoint(camera->index, p3d[p3]);

						// Save triangle points to the output file
						outC << vertex1.x << " " << vertex1.y << " " << vertex1.z << " " << p1_new.x << " " << p1_new.y << endl;
						outC << vertex2.x << " " << vertex2.y << " " << vertex2.z << " " << p2_new.x << " " << p2_new.y << endl;
						outC << vertex3.x << " " << vertex3.y << " " << vertex3.z << " " << p3_new.x << " " << p3_new.y << endl;
					 }
				}
			}
		}
		offset += NoP[xx]; // Update offset
	}
	outC.close(); // Close file
}


/**************************************************************************************************************
 *
 * @brief  			Rotate point according to the camera index
 *
 * @param  in		index - camera index
 * 		   in		Point3f point - grid point
 *
 * @return 			Point3f point - point after rotation
 *
 * @remarks 		The function rotate grid point according to the camera index value:
 * 						index = 0 - without rotation;
 * 						index = 1 - 90 degree clockwise rotation;
 * 						index = 2 - 180 degree clockwise rotation;
 * 						index = 3 - 270 degree clockwise rotation;
 *
 **************************************************************************************************************/
Point3f rotatePoint(int index, Point3f point)
{
	Point3f result;
	switch(index)
	{
	case(0): // Without rotation
		result.x =   point.x;
		result.y = - point.y;
		result.z = - point.z;
		break;
	case(1): // 90 degree clockwise rotation
		result.x = - point.y;
		result.y = - point.x;
		result.z = - point.z;
		break;
	case(2): // 180 degree clockwise rotation
		result.x = - point.x;
		result.y =   point.y;
		result.z = - point.z;
		break;
	default: // case(3)    270 degree clockwise rotation
		result.x =   point.y;
		result.y =   point.x;
		result.z = - point.z;
		break;
	}
	return(result);
}
