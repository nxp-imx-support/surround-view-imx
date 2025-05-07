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

#include "exposure_compensator.hpp"

/**************************************************************************************************************
 *
 * @brief  			Compensator class constructor.
 *
 * @param  in 		Size mask_size	-	size of compensator mask (screen size)
 *
 * @return 			The function create the Compensator object.
 *
 * @remarks 		The function sets cinf.roi_mask property of new Compensator object.
 *
 **************************************************************************************************************/
Compensator::Compensator(Size mask_size)
{
	cinf.roi_mask = Mat(mask_size, CV_8UC1, Scalar(0));
}

/**************************************************************************************************************
 *
 * @brief  			Get coordinates of rectangle reflection across the x-axis.
 *
 * @param  in 		uint index	-	rectangle index
 *
 * @return 			Rect - rectangle reflection across the x-axis.
 *
 * @remarks 		The function reflect rectangle across the x-axis.
 *
 **************************************************************************************************************/
Rect Compensator::getFlipROI(uint index) 
{
	if (index < cinf.roi.size())
	{
		float x1 = (cinf.roi[index].x + 1.0f) * (float)cinf.roi_mask.cols / 2.0f;
		float y1 = (-cinf.roi[index].y + 1.0f) * (float)cinf.roi_mask.rows / 2.0f;
		float x2 = (cinf.roi[index].x + cinf.roi[index].width + 1.0f) * (float)cinf.roi_mask.cols / 2.0f;
		float y2 = (-cinf.roi[index].y - cinf.roi[index].height + 1.0f) * (float)cinf.roi_mask.rows / 2.0f;
		return Rect(Point((int)x1, (int)y1), Point((int)x2, (int)y2));
	}
	else {
		return Rect(0, 0, 0, 0);
	}
}	

/**************************************************************************************************************
 *
 * @brief  			Fill the CompensatorInfo vector private property of Compensator object.
 *
 * @param  	in		vector<Camera*> cameras - vector of Camera objects
 *
 * @return 			-
 *
 * @remarks 		The function fills CompensatorInfo property of Compensator. It calculates mask which
 * 					defines 4 overlap regions and circumscribed rectangles of each region.
 *
 **************************************************************************************************************/
#ifdef CAMERA_HPP_EXIST
void Compensator::feed(vector<Camera*> &cameras, vector< vector<Point3f> > &seam_points)
{
	for(uint i = 0; i < cameras.size(); i++) {
		if(seam_points[i].size() < 8)
		{
			cout << "Exposure compensator was not generated. Seam " << i << " doesn't contain 8 points" << endl;
			return;
		}
	}

	cinf.radius = sqrt(pow(seam_points[0][1].x, 2) + pow(seam_points[0][1].y, 2));

	double height = (double)cinf.roi_mask.rows / (double)cinf.radius / 2.0;
	double x_gain = 2.0 * cinf.radius;
	double y_gain = 2.0 * cinf.radius * (double)cinf.roi_mask.rows / (double)cinf.roi_mask.cols;

	for(size_t i = 0; i < cameras.size(); i+=2U)
	{
		vector<Point> p_left, p_right;
		double sg = (double)pow(-1.0f, (int)(i >> 1U));
		int i_next = next_id((int)i, (int)cameras.size() - 1); // Index of the previous camera
		int i_prev = previous_id((int)i, (int)cameras.size() - 1); // Index of the next camera

		p_left.push_back(Point2f((float)((sg * (double)seam_points[i][0].x + cinf.radius) * height), (float)((sg * (double)seam_points[i][0].y + cinf.radius) * height)));
		p_left.push_back(Point2f((float)((sg * (double)seam_points[i][1].x + cinf.radius) * height), (float)((sg * (double)seam_points[i][1].y + cinf.radius) * height)));
		p_left.push_back(Point2f((float)((sg * (double)seam_points[i_prev][6].y + cinf.radius) * height), (float)((- sg * (double)seam_points[i_prev][6].x + cinf.radius) * height)));
		p_left.push_back(Point2f((float)((sg * (double)seam_points[i_prev][7].y + cinf.radius) * height), (float)((- sg * (double)seam_points[i_prev][7].x + cinf.radius) * height)));

		p_right.push_back(Point2f((float)((- sg * seam_points[i_next][0].y + cinf.radius) * height), (float)((sg * seam_points[i_next][0].x + cinf.radius) * height)));
		p_right.push_back(Point2f((float)((- sg * seam_points[i_next][1].y + cinf.radius) * height), (float)((sg * seam_points[i_next][1].x + cinf.radius) * height)));
		p_right.push_back(Point2f((float)((sg * seam_points[i][6].x + cinf.radius) * height), (float)((sg * seam_points[i][6].y + cinf.radius) * height)));
		p_right.push_back(Point2f((float)((sg * seam_points[i][7].x + cinf.radius) * height), (float)((sg * seam_points[i][7].y + cinf.radius) * height)));

		cinf.roi.push_back(Rect2f(Point2f((float)(sg * (double)seam_points[i][1].x / x_gain), (float)(- sg * (double)seam_points[i_prev][6].x / y_gain)),
								  Point2f((float)(sg * (double)seam_points[i_prev][7].y / x_gain), (float)(sg * (double)seam_points[i][0].y / y_gain))));
		cinf.roi.push_back(Rect2f(Point2f((float)(- sg * (double)seam_points[i_next][0].y / x_gain), (float)(sg * (double)seam_points[i][7].y / y_gain)),
								  Point2f((float)(sg * (double)seam_points[i][6].x / x_gain), (float)(sg * (double)seam_points[i_next][1].x / y_gain))));

		fillConvexPoly(cinf.roi_mask, p_left, Scalar(255)); // Draws a filled convex polygon using all seam points
		fillConvexPoly(cinf.roi_mask, p_right, Scalar(255)); // Draws a filled convex polygon using all seam points
	}
}
#endif

/**************************************************************************************************************
 *
 * @brief  			Save compensator info
 *
 * @param  	in		char* path - path name
 *
 * @return 			-
 *
 * @remarks 		The function generates grids only for overlap regions which lays on flat bowl bottom for each
 * 					camera and saves the grids into "compensator" folder - 1 grid per camera. Each grid contained
 * 					description of two overlap regions: left and right.
 *					Also the application save the file with texel coordinates of circumscribed rectangle for each
 *					overlap region. The texture mapping application uses this information to copy only overlap
 *					regions from frame buffer when it calculates exposure correction coefficients.
 *
 **************************************************************************************************************/
int Compensator::save(const char* path)
{
	double x_gain = 2.0 * cinf.radius;
	double y_gain = 2.0 * cinf.radius * (double)cinf.roi_mask.rows / (double)cinf.roi_mask.cols;
	double height = (double)cinf.roi_mask.rows / (double)cinf.radius / 2.0;

	struct stat st = {0};
	if(stat(path, &st) == -1) { // Check if the 'path' path exists
		if(mkdir(path, 0x0700) != 0) // Create 'path' path if doesn't exist
		{
			cout << "mkdir: cannot create directory " << path << endl;
			return(-1);
		}
	}

	for(uint i = 0; i < cinf.roi.size(); i++)
	{
		char file_name[50], file_name_roi[50];
		sprintf(file_name, "./array%d", i + 1U);
		sprintf(file_name_roi, "%s/array%d", path, i + 1U);

		ifstream ifs_ref(file_name);
		if(ifs_ref) // // The file exists, and is open for input
		{
			ofstream outC; // Output file
			outC.open(file_name_roi, std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.

			float vx[3], vy[3], vz[3], tx[3], ty[3];
			while (ifs_ref >> vx[0] >> vy[0] >> vz[0] >> tx[0] >> ty[0] >> vx[1] >> vy[1] >> vz[1] >> tx[1] >> ty[1] >> vx[2] >> vy[2] >> vz[2] >> tx[2] >> ty[2] ) // Read triangle
			{
				if((vz[0] == 0.0f) && (vz[1] == 0.0f) && (vz[2] == 0.0f))
				{
					double x0 = (vx[0] + cinf.radius) * height;
					double y0 = (- vy[0] + cinf.radius) * height;

					double x1 = (vx[1] + cinf.radius) * height;
					double y1 = (- vy[1] + cinf.radius) * height;

					double x2 = (vx[2] + cinf.radius) * height;
					double y2 = (- vy[2] + cinf.radius) * height;


					if((x0 >= (double)0.0) && (x0 < (double)cinf.roi_mask.cols) && (y0 >= (double)0.0) && (y0 < (double)cinf.roi_mask.rows) &&
					   (x1 >= (double)0.0) && (x1 < (double)cinf.roi_mask.cols) && (y1 >= (double)0.0) && (y1 < (double)cinf.roi_mask.rows) &&
					   (x2 >= (double)0.0) && (x2 < (double)cinf.roi_mask.cols) && (y2 >= (double)0.0) && (y2 < (double)cinf.roi_mask.rows))

					{
						uint vertexes_sum = (uint)cinf.roi_mask.at<uchar>(Point(x0, y0)) +
											(uint)cinf.roi_mask.at<uchar>(Point(x1, y1)) +
											(uint)cinf.roi_mask.at<uchar>(Point(x2, y2));

						if(vertexes_sum == 765U) // 3 * 255
						{
							outC << vx[0] / x_gain << " " << vy[0] / y_gain << " " << vz[0] << " " << tx[0] << " " << ty[0] << endl;
							outC << vx[1] / x_gain << " " << vy[1] / y_gain << " " << vz[1] << " " << tx[1] << " " << ty[1] << endl;
							outC << vx[2] / x_gain << " " << vy[2] / y_gain << " " << vz[2] << " " << tx[2] << " " << ty[2] << endl;
						}
					}
				}
			}
			outC.close(); // Close file
		}
		else
		{
			cout << "Compensator grids have not been saved. File " << file_name << " not found" << endl;
			return(-1);
		}
	}

	char file_name[50];
	sprintf(file_name, "%s/compensator", path);
	ofstream outC; // Output file
	outC.open(file_name, std::ofstream::out | std::ofstream::trunc); // Any contents that existed in the file before it is open are discarded.
	for(uint i = 0; i < cinf.roi.size(); i++)
	{
		outC << cinf.roi[i].x << " " << cinf.roi[i].y << " ";
		outC << cinf.roi[i].x + cinf.roi[i].width << " " << cinf.roi[i].y + cinf.roi[i].height << endl;
	}
	outC.close(); // Close file

	return(0);
}

/**************************************************************************************************************
 *
 * @brief  			Load compensator info
 *
 * @param  	in		const char* path - path name
 *
 * @return 			-
 *
 * @remarks 		The function loads texel coordinates of circumscribed rectangle for each overlap region.
 * 					The texture mapping application uses this information to copy only overlap regions from
 * 					frame buffer when it calculates exposure correction coefficients.
 *
 **************************************************************************************************************/
int Compensator::load(const char* path)
{
	char file_name[50];
	sprintf(file_name, "%s/compensator", path);
	ifstream ifs_ref(file_name);
	if(ifs_ref) // The file exists, and is open for input
	{
		cinf.roi.clear();
		float p1, p2, p3, p4;
		while (ifs_ref >> p1 >> p2 >> p3 >> p4 ) // Read triangle
		{
			cinf.roi.push_back(Rect2f(Point2f(p1, p2), Point2f(p3, p4)));

			vector<Point> p;
			p.push_back(Point2f((p1 + 1.0f) * (float)cinf.roi_mask.cols / 2.0f, (p2 + 1.0f) * (float)cinf.roi_mask.rows / 2.0f));
			p.push_back(Point2f((p3 + 1.0f) * (float)cinf.roi_mask.cols / 2.0f, (p2 + 1.0f) * (float)cinf.roi_mask.rows / 2.0f));
			p.push_back(Point2f((p3 + 1.0f) * (float)cinf.roi_mask.cols / 2.0f, (p4 + 1.0f) * (float)cinf.roi_mask.rows / 2.0f));
			p.push_back(Point2f((p1 + 1.0f) * (float)cinf.roi_mask.cols / 2.0f, (p4 + 1.0f) * (float)cinf.roi_mask.rows / 2.0f));
			fillConvexPoly(cinf.roi_mask, p, Scalar(255)); // Draws a filled convex polygon using all seam points
		}
	}
	else
	{
		cout << "Compensator has not been loaded. File " << file_name << " not found" << endl;
		return(-1);
	}
	return(0);
}

