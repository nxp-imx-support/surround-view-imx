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


#ifndef SRC_DEFISHEYE_HPP_
#define SRC_DEFISHEYE_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <opencv2/highgui/highgui.hpp>

using namespace std;
using namespace cv;

/*******************************************************************************************
 * Types
 *******************************************************************************************/
struct camera_model
{
	vector<double>	pol;		/* The polynomial coefficients of radial camera model */
	vector<double>	invpol;		/* The coefficients of the inverse polynomial */
	Point2d			center;		/* x and y coordinates of the center in pixels */
	Matx22d			affine;		/* | sx  shy | sx, sy - scale factors along the x/y axis
	                               | shx sy  | shx, shy -  shear factors along the x/y axis */
	Size			img_size;	/* The image size (width/height) */
};


/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* Defisheye class */
class Defisheye {
public:
	camera_model model;
	int loadModel(string filename);
	void createLUT(Mat &mapx, Mat &mapy, float sf);
	
	
	void cam2world(Point3d* p3d, Point2d p2d);
	
private:

};
#endif /* SRC_DEFISHEYE_H */
