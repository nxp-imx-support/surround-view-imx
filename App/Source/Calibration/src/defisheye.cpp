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
#include "defisheye.hpp"

int Defisheye::loadModel(string filename)
{
	int return_val = 0;
	struct stat st;
	if (stat(filename.c_str(), &st) != 0) // Get filename file information and check if statistics are valid
	{
		cout << "File " << filename << " not found" << endl;
		return_val = -1;
	}
	else {
		string line;
		ifstream ifs_ref(filename.c_str());
		int line_num = 0;
		while (getline(ifs_ref, line))
		{
			if ((line.length() > 1) && (line.at(0) != '#')) 
			{
				stringstream str_stream(line);
				double dbl_val;
			
				switch (line_num)
				{
				case 0:		// polynomial coefficients
					str_stream >> dbl_val;
					while (str_stream >> dbl_val) {
						model.pol.push_back(dbl_val);
					}
					break;
				case 1:		// polynomial coefficients for the inverse mapping function
					str_stream >> dbl_val;
					while (str_stream >> dbl_val) {
						model.invpol.push_back(dbl_val);
					}
					break;
				case 2:		// center
					str_stream >> model.center.x;
					str_stream >> model.center.y;
					break;
				case 3:		// affine parameters
					str_stream >> model.affine(0, 0);
					str_stream >> model.affine(0, 1);
					str_stream >> model.affine(1, 0);
					model.affine(1, 1) = 1.0;
					break;
				case 4:		// image size
					str_stream >> model.img_size.height;
					str_stream >> model.img_size.width;
					break;
				default:
					return_val = -1;
					break;					
				}
				line_num++;
			}
		}
		ifs_ref.close();
	}

	return return_val;	
}



void Defisheye::createLUT(Mat &mapx, Mat &mapy, float sf)
{
	Point3d p3D; // X, Y, Z

	mapx.create(model.img_size.height, model.img_size.width, CV_32FC1);
	mapy.create(model.img_size.height, model.img_size.width, CV_32FC1);
	
	float xc_norm = (float)model.img_size.width / 2.0f;
	float yc_norm = (float)model.img_size.height / 2.0f;
	p3D.z = (double)(-model.img_size.width) / sf;  // Z
    
	for (int col = 0; col < model.img_size.width; col++) {
		for (int row = 0; row < model.img_size.height; row++)
		{   
			p3D.x = (double)row - (double)yc_norm; // X
			p3D.y = (double)col - (double)xc_norm; // Y
			
			// norm = sqrt(X^2 + Y^2)
			double norm = sqrt(p3D.x * p3D.x + p3D.y * p3D.y);
			if (norm == 0.0)
			{
				mapx.at<float>(row, col) = (float) model.center.y;
				mapy.at<float>(row, col) = (float) model.center.x;	
				continue;
			}
						
			// t = atan(Z/sqrt(X^2 + Y^2)) 
			double t = atan(p3D.z / norm);
			
			// r = a0 + a1 * t + a2 * t^2 + a3 * t^3 + ...
			double t_pow = t;
			double r = model.invpol[0];
			
			for (uint i = 1; i < model.invpol.size(); i++)
			{				
				r += t_pow * model.invpol[i];
				t_pow *= t;
			}
			
			/* | u | = r * | X | / sqrt(X^2 + Y^2);
			   | v |       | Y |                     */
			double u = r * p3D.x / norm;
			double v = r * p3D.y / norm;
			
			/* | x | = | sx  shy | * | u | + | xc |
			   | y |   | shx  1  |   | v |   | yc |     */
			mapy.at<float>(row, col) = (float)((model.affine(0, 0) * u + model.affine(0, 1) * v + model.center.x));
			mapx.at<float>(row, col) = (float)((model.affine(1, 0) * u + model.affine(1, 1) * v + model.center.y));
		}
	}
}













void Defisheye::cam2world(Point3d* p3d, Point2d p2d)
{
	double invdet  = 1.0 / (model.affine(0, 0) - model.affine(0, 1) * model.affine(1, 0)); // 1/det(A), where A = [c,d;e,1] as in the Matlab file

	double xp = invdet*((p2d.x - model.center.x) - model.affine(0, 1) * (p2d.y - model.center.y));
	double yp = invdet*(-model.affine(1, 0) * (p2d.x - model.center.x) + model.affine(0, 0) * (p2d.y - model.center.y));
  
	double r   = sqrt(xp*xp + yp*yp); //distance [pixels] of  the point from the image center
	double zp  = model.pol[0];
	double r_i = 1.0;
	 
	for (uint i = 1; i < model.pol.size(); i++)
	{
		r_i *= r;
		zp += r_i * model.pol[i];
	}
 
	//normalize to unit norm
	double invnorm = 1.0 / sqrt(xp*xp + yp*yp + zp*zp);
 
	p3d->x = invnorm*xp;
	p3d->y = invnorm*yp; 
	p3d->z = invnorm*zp;
}
