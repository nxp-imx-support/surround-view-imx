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


#include "masks.hpp"

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
void Masks::createMasks(vector<Camera*> &cameras, vector< vector<Point3f> > &seam_points, float smothing)
{
	for(uint i = 0; i < cameras.size(); i++) {
		if(seam_points[i].size() < 8)
		{
			cout << "Masks was not generated. Seam " << i << " doesn't contain 8 points" << endl;
			return;
		}
	}



	// Get grids intersection points
	for(uint i = 0; i < cameras.size(); i++)
	{
		Seam seam_left, seam_right;
		getSeamPoints(seam_points[i], seam_points[next_id((int)i, (int)cameras.size() - 1)], seam_right, 1); // intersection with next grid
		seamr.push_back(seam_right);
		getSeamPoints(seam_points[i], seam_points[previous_id((int)i, (int)cameras.size() - 1)], seam_left, -1); // intersection with previous grid
		seaml.push_back(seam_left);
	}

	// Calculate seams
	for(uint i = 0; i < cameras.size(); i++)
	{
		vector<Point3f> seam3d;
		double radius = sqrt(pow(seam_points[i][1].x, 2) + pow(seam_points[i][1].y, 2));
		// Left edge
		seaml[i].p2.x = static_cast<float>((- seaml[i].line.alpha * seaml[i].line.beta - sqrt(pow(radius, 2) * (1.0 + pow(seaml[i].line.alpha, 2)) - pow(seaml[i].line.beta, 2))) / (1.0 + pow(seaml[i].line.alpha, 2)));
		seaml[i].p2.y = (float)sqrt(pow(radius, 2) - pow(seaml[i].p2.x, 2));
		double cos_l = seaml[i].p2.x / radius;
		// Right edge
		seamr[i].p2.x = static_cast<float>((- seamr[i].line.alpha * seamr[i].line.beta + sqrt(pow(radius, 2) * (1.0 + pow(seamr[i].line.alpha, 2)) - pow(seamr[i].line.beta, 2))) / (1.0 + pow(seamr[i].line.alpha, 2)));
		seamr[i].p2.y = (float)sqrt(pow(radius, 2) - pow(seamr[i].p2.x, 2));
		double cos_r = seamr[i].p2.x / radius;

		// Left horizontal seam
		for(double xx = seaml[i].p1.x; xx >= seaml[i].p2.x; xx-=SEAM_STEP) {
			seam3d.push_back(Point3f(xx, xx * seaml[i].line.alpha + seaml[i].line.beta, 0.0));
		}

		// Left vertical seam
		for(double zz = 0.0; zz < abs(seam_points[i][3].z); zz+=SEAM_STEP)
		{
			double new_x = (radius + sqrt(zz)) * cos_l;
			double new_y = - new_x * tan(acos(cos_l));
			seam3d.push_back(Point3f(new_x, new_y, - zz));
		}

		// Top seam
		for(double aa = cos_l; aa < cos_r; aa+=SEAM_STEP)
		{
			double new_x = (radius + sqrt(abs(seam_points[i][3].z))) * aa;
			double new_y = - new_x * tan(acos(aa));
			seam3d.push_back(Point3f(new_x, new_y, seam_points[i][3].z));
		}

		// Right vertical seam
		for(double zz = abs(seam_points[i][4].z); zz > 0.0; zz-=SEAM_STEP)
		{
			double new_x = (radius + sqrt(zz)) * cos_r;
			double new_y = - new_x * tan(acos(cos_r));
			seam3d.push_back(Point3f(new_x, new_y, - zz));
		}

		// Right horizontal seam
		for(double xx = seamr[i].p2.x; xx >= seamr[i].p1.x; xx-=SEAM_STEP) {
			seam3d.push_back(Point3f(xx, xx * seamr[i].line.alpha + seamr[i].line.beta, 0.0));
		}

		// Bottom seam
		Line bottom = getAlphaBeta(seamr[i].p1, seaml[i].p1);
		for(double xx = seamr[i].p1.x; xx > seaml[i].p1.x; xx-=SEAM_STEP)
		{
			seam3d.push_back(Point3f(xx, bottom.alpha * xx + bottom.beta, 0));
		}

		// Projects 3D seam points to an image plane
		vector<Point2f> seam2d;
		projectPoints(seam3d, cameras[i]->getRvec(), cameras[i]->getTvec(), cameras[i]->getK(), cameras[i]->getDistCoeffs(), seam2d);

		// Get seam for fisheye image
		vector<Point> seam;
		for(uint j = 0; j < seam2d.size(); j++)
		{
			if((seam2d[j].x < (float)cameras[i]->xmap.cols - 1.0) && (seam2d[j].y < (float)cameras[i]->xmap.rows - 1.0) && (seam2d[j].x > 0.0) && (seam2d[j].y > 0.0))
			{
				seam.push_back(Point2f(cameras[i]->xmap.at<float>(seam2d[j]), cameras[i]->ymap.at<float>(seam2d[j])));
			}
		}
		seam.push_back(seam[0]);

		// Create masks which are limited to seams.
		Mat cam_mask(cameras[i]->xmap.rows, cameras[i]->xmap.cols, CV_8UC1, Scalar(0));
		masks.push_back(cam_mask);
		fillConvexPoly(masks[i], seam, Scalar(255)); // Draws a filled convex polygon using all seam points

		// Define left edge of smoothing
		if((smothing > 0.5f) || (smothing < 0.0f)) { smothing = 0.5f; }
		double smoothing_angle = atan(seaml[i].line.alpha);
		vector<Point3f> left_points;
		left_points.push_back(Point3f(seaml[i].p1.x, seaml[i].p1.y, 0));
		left_points.push_back(Point3f(seaml[i].p2.x, sqrt(pow(radius, 2) - pow(seaml[i].p2.x, 2)), 0));
		left_points.push_back(seam_points[i][3]);
		smoothMaskEdge(masks[i], Vec2b(0, 255), Vec2d(smoothing_angle - smothing, smoothing_angle + smothing), 0.0039, left_points, cameras[i]);

		// Define right edge of smoothing
		smoothing_angle = atan(seamr[i].line.alpha);
		vector<Point3f> right_points;
		right_points.push_back(Point3f(seamr[i].p1.x, seamr[i].p1.y, 0));
		right_points.push_back(Point3f(seamr[i].p2.x, sqrt(pow(radius, 2) - pow(seamr[i].p2.x, 2)), 0));
		right_points.push_back(seam_points[i][4]);
		smoothMaskEdge(masks[i], Vec2b(255, 0), Vec2d(smoothing_angle - smothing, smoothing_angle + smothing), 0.0039, right_points, cameras[i]);

		// Blurs an image using a Gaussian filter
		for (int row = 0; row < masks[i].rows; row++) {
			Mat roi = cam_mask(Rect(0, row, masks[i].cols, 1));
			GaussianBlur(roi, roi, Size(2 * ((masks[i].rows - row) / 2) + 1, 1), 10.0, 1.0);
		}
		GaussianBlur(masks[i], masks[i], Size(3, 3), 10.0);


		// Save masks
		char mask_name[50];
		sprintf(mask_name, "./mask%d.jpg", i);
		if (imwrite(mask_name, masks[i]) == false) {
			cout << "Failed to save " << mask_name << endl;
		}
	}
}

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
 * 					and pruduces two output grids: first for overlapping regions ("arrayX1") and second for
 * 					non-overlapping regions ("arrayX2").
 * 					Camera blending mask is used to split grid into two parts. The mask value has been checked
 * 					for each texels of a rendered triangle. If at least one of texels is masked with value less than
 * 					255 then the triangle is written to overlap grid. Otherwise the triangle is written to non-overlap
 * 					grid.
 *
 **************************************************************************************************************/
int Masks::splitGrids(void)
{
	float vx[3], vy[3], vz[3], tx[3], ty[3];

	for(uint i = 0; i < masks.size(); i++)
	{
		char file_name[50], file_name_b[50], file_name_wb[50];
		sprintf(file_name, "./array%d", i + 1U);		// Name of input grid
		sprintf(file_name_b, "./array%d1", i + 1U); 	// Name of overlap grid
		sprintf(file_name_wb, "./array%d2", i + 1U); // Name of non-overlap grid

		ifstream ifs_ref(file_name);
		if(ifs_ref) // // The file exists, and is open for input
		{
			ofstream outC_b, outC_wb; // Output file
			outC_b.open(file_name_b, std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.
			outC_wb.open(file_name_wb, std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.

			while (ifs_ref >> vx[0] >> vy[0] >> vz[0] >> tx[0] >> ty[0] >> vx[1] >> vy[1] >> vz[1] >> tx[1] >> ty[1] >> vx[2] >> vy[2] >> vz[2] >> tx[2] >> ty[2] ) // Read triangle
			{
				uint pixels_sum = 0U; // Sum of pixels of triangle vertexes
				for(int j = 0; j < 3; j ++) // For each vertexes of the triangle
				{
					Point idx1 = Point(static_cast<int>(tx[j] * (float)masks[i].cols) - 40, static_cast<int>(ty[j] * (float)masks[i].rows) - 40);
					Point idx2 = Point(static_cast<int>(tx[j] * (float)masks[i].cols) + 40, static_cast<int>(ty[j] * (float)masks[i].rows) + 40);
										
					if ((idx1.x < masks[i].cols) && (idx1.y < masks[i].rows) && (idx1.x > 0) && (idx1.y > 0))
					{
						pixels_sum += masks[i].at<uchar>(idx1); // Add pixel value
						if((idx2.x < masks[i].cols) && (idx2.y < masks[i].rows) && (idx2.x > 0) && (idx2.y > 0))
						{
							pixels_sum += masks[i].at<uchar>(Point(idx2.x, idx2.y)); // Add pixel value
							pixels_sum += masks[i].at<uchar>(Point(idx1.x, idx2.y)); // Add pixel value
							pixels_sum += masks[i].at<uchar>(Point(idx2.x, idx1.y)); // Add pixel value
						}
					}	
					else {
						if((idx2.x < masks[i].cols) && (idx2.y < masks[i].rows) && (idx2.x > 0) && (idx2.y > 0))
						{
							pixels_sum += masks[i].at<uchar>(idx2); // Add pixel value
						}
					}
					
				}

				if(pixels_sum == 4U * 765U) // If all 3 vertexes of triangles are white (3 * 255 = 765) -> non-overlap region
				{
					outC_wb << vx[0] << " " << vy[0] << " " << vz[0] << " " << tx[0] << " " << ty[0] << endl;
					outC_wb << vx[1] << " " << vy[1] << " " << vz[1] << " " << tx[1] << " " << ty[1] << endl;
					outC_wb << vx[2] << " " << vy[2] << " " << vz[2] << " " << tx[2] << " " << ty[2] << endl;
				}
				else {
					if(pixels_sum != 0U) // Otherwise -> overlap region
					{
						outC_b << vx[0] << " " << vy[0] << " " << vz[0] << " " << tx[0] << " " << ty[0] << endl;
						outC_b << vx[1] << " " << vy[1] << " " << vz[1] << " " << tx[1] << " " << ty[1] << endl;
						outC_b << vx[2] << " " << vy[2] << " " << vz[2] << " " << tx[2] << " " << ty[2] << endl;
					}
				}
			}
			outC_b.close(); // Close file
			outC_wb.close(); // Close file
		}
		else
		{
			cout << "Texels/vertices grids have not been split. File " << file_name << " not found" << endl;
			return(-1);
		}
	}
	return(0);
}


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
void Masks::smoothMaskEdge(Mat &img, Vec2b colors, Vec2d angles, double angle_step, vector<Point3f> edge_points, Camera* camera)
{
	double color = (double)colors[0];
	double color_step = static_cast<double>(colors[1] - colors[0]) * angle_step / (angles[1] - angles[0]);
	double radius = sqrt(pow(edge_points[1].x, 2) + pow(edge_points[1].y, 2));

	double x_start, x_end, angle_cos;

	for(double ang = angles[0]; ang < angles[1]; ang+=angle_step)
	{
		double alpha = tan(ang);
		double beta = edge_points[0].y - alpha * edge_points[0].x;

		if(alpha > 0.0)
		{
			x_start = (- alpha * beta - sqrt(pow(radius, 2) * (1 + pow(alpha, 2)) - pow(beta, 2))) / (1 + pow(alpha, 2));
			x_end = edge_points[0].x;
			angle_cos = x_start / radius;
		}
		else
		{
			x_start = edge_points[0].x;
			x_end = (- alpha * beta + sqrt(pow(radius, 2) * (1 + pow(alpha, 2)) - pow(beta, 2))) / (1 + pow(alpha, 2));
			angle_cos = x_end / radius;
		}

		vector<Point3f> seam3d;
		vector<Point2f> seam2d, seam;

		// Horizontal part of seam
		for(double xx = x_start; xx <= x_end; xx+=SEAM_STEP) {
			seam3d.push_back(Point3f(xx, xx * alpha + beta, 0.0));
		}

		// Vertical part of seam
		for(double zz = 0.0; zz < abs(edge_points[2].z); zz+=SEAM_STEP)
		{
			double new_x = (radius + sqrt(zz)) * angle_cos;
			double new_y = - new_x * tan(acos(angle_cos));
			seam3d.push_back(Point3f(new_x, new_y, - zz));
		}

		projectPoints(seam3d, camera->getRvec(), camera->getTvec(), camera->getK(), camera->getDistCoeffs(), seam2d);


		for(uint j = 0; j < seam2d.size(); j++)
		{
			if((seam2d[j].x < (float)camera->xmap.cols - 1.0f) && (seam2d[j].y < (float)camera->xmap.rows - 1.0f) && (seam2d[j].x > 0.0f) && (seam2d[j].y > 0.0f))
			{
					seam.push_back(Point2f(camera->xmap.at<float>(seam2d[j]), camera->ymap.at<float>(seam2d[j])));
					circle(img, seam[j], 1, color, 1);
			}
		}
		color += color_step;
	}
}


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
void Masks::getSeamPoints(vector<Point3f> &polygon1, vector<Point3f> &polygon2, Seam &seam, int rotation)
{
	bool intersection = false;


	// Search for the first intersection point of input polygons
	int pnt_1 = (int)polygon1.size() - 2; // 7th seam point
	while((pnt_1 < (int)polygon1.size()) && (!intersection)) // pnt_1 can be only 6 and 7 (7th and 8th polygon points)
	{
		int pnt_2 = 1;
		while((pnt_2 >= 0) && (!intersection)) // pnt_2 can be only 1 and 0 (1st and 2nd polygon points)
		{
			Point2f p1_1, p1_2, p2_1, p2_2;
			if(rotation < 0) // Rotate polygon2 points to the left
			{
				// 1st polygon, side 2-1 or 1-8
				p1_1 = Point2f(polygon1[pnt_2].x, polygon1[pnt_2].y);
				p1_2 = Point2f(polygon1[previous_id(pnt_2, (int)polygon2.size() - 1)].x, polygon1[previous_id(pnt_2, (int)polygon2.size() - 1)].y);
				// 2nd polygon, side 7-8 or 8-1
				p2_1 = Point2f(polygon2[pnt_1].y, - polygon2[pnt_1].x);
				p2_2 = Point2f(polygon2[next_id(pnt_1, (int)polygon2.size() - 1)].y, - polygon2[next_id(pnt_1, (int)polygon2.size() - 1)].x);
			}
			else // Rotate polygon2 points to the right
			{
				// 1st polygon, side 7-8 or 8-1
				p1_1 = Point2f(polygon1[pnt_1].x, polygon1[pnt_1].y);
				p1_2 = Point2f(polygon1[next_id(pnt_1, (int)polygon1.size() - 1)].x, polygon1[next_id(pnt_1, (int)polygon1.size() - 1)].y);
				// 2nd polygon, side 1-0 or 0-8
				p2_1 = Point2f(- polygon2[pnt_2].y, polygon2[pnt_2].x);
				p2_2 = Point2f(- polygon2[previous_id(pnt_2, (int)polygon2.size() - 1)].y, polygon2[previous_id(pnt_2, (int)polygon2.size() - 1)].x);
			}

			// Calculate polygon sides intersection
			Line line1 = getAlphaBeta(p1_1, p1_2);
			Line line2 = getAlphaBeta(p2_1, p2_2);
			seam.p1 = getIntersection(line1, line2);

			// Check if intersection point lays on a polygon side
			if((seam.p1.x > MIN(p2_1.x, p2_2.x)) && (seam.p1.y > MIN(p2_1.y, p2_2.y)) &&
			   (seam.p1.x < MAX(p2_1.x, p2_2.x)) && (seam.p1.y < MAX(p2_1.y, p2_2.y)) &&
			   (seam.p1.x > MIN(p1_1.x, p1_2.x)) && (seam.p1.y > MIN(p1_1.y, p1_2.y)) &&
			   (seam.p1.x < MAX(p2_1.x, p2_2.x)) && (seam.p1.y < MAX(p2_1.y, p2_2.y)))

			{
				intersection = true; // Terminate searching process
				double radius;
				if(rotation < 0) // left
				{
					radius = MAX(pow(polygon1[0].y, 2) + pow(polygon1[0].x, 2),
								 pow(polygon2[polygon2.size() - 1].y, 2) + pow(polygon2[polygon2.size() - 1].x, 2));
				}
				else // right
				{
					radius = MAX(pow(polygon1[polygon1.size() - 1].y, 2) + pow(polygon1[polygon1.size() - 1].x, 2),
								 pow(polygon2[0].y, 2) + pow(polygon2[0].x, 2));
				}
				// Check if intersection point belongs to the 1st and 2nd grid which don't cover whole polygons area
				if(radius > ((double)seam.p1.x * seam.p1.x + (double)seam.p1.y * seam.p1.y))
				{
					double alpha = (double)seam.p1.y / seam.p1.x;
					if(seam.p1.x < 0.0) {
						seam.p1.x = -1.0f * (float)sqrt(radius / (1.0 + (alpha * alpha)));
					}
					else {
						seam.p1.x = (float)sqrt(radius / (1.0 + (alpha * alpha)));
					}
					seam.p1.y = (float)alpha * seam.p1.x;
				}
			}
			pnt_2--; // 1st seam point
		}
		pnt_1++; // 8th seam point
	}


	// Search for the second intersection point of input polygons
	Point2f p1_4, p1_5, p2_4, p2_5;
	// 1st polygon, side 4-5
	p1_4 = Point2f(polygon1[3].x, polygon1[3].y);
	p1_5 = Point2f(polygon1[4].x, polygon1[4].y);

	// 2st polygon, side 4-5
	if(rotation < 0) // Rotate polygon2 points to the left
	{
		p2_4 = Point2f(polygon1[3].y, - polygon1[3].x);
		p2_5 = Point2f(polygon1[4].y, - polygon1[4].x);
	}
	else // Rotate polygon2 points to the right
	{
		p2_4 = Point2f(- polygon2[3].y, polygon2[3].x);
		p2_5 = Point2f(- polygon2[4].y, polygon2[4].x);
	}

	// Calculate polygon sides intersection
	Line line1 = getAlphaBeta(p1_4, p1_5);
	Line line2 = getAlphaBeta(p2_4, p2_5);
  	seam.p2 = getIntersection(line1, line2);

	seam.line = getAlphaBeta(seam.p1, seam.p2);
}
