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


#ifndef SRC_SETTINGS_HPP_
#define SRC_SETTINGS_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <boost/functional/hash.hpp>
#include <boost/lexical_cast.hpp>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

using namespace boost;
using namespace std;

/*******************************************************************************************
 * Types
 *******************************************************************************************/
struct CamParam {		/* Camera settings */
	int height;			/* Camera frame height in pixels */
	int width;			/* Camera frame width in pixels */
	float sf;			/* Defisheye scale factor */
	int roi;			/* Region of interest for contours searching. 
						   In percent of input frame height from bottom */
	int cntr_min_size;	/* Minimum length of contour in pixels */
	int chessboard_num;	/* Number of chessboard images which will be used for camera calibraton */
	string device;		/* Device name (/dev/videoX) */
};

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* XMLParameters class */
class XMLParameters {
	public:
		// Pathes
		string camera_inputs;	/* Path to camera calibration frame files */
		string camera_models;	/* Path to camera model files */
		string tmplt;			/* Path to template points files */
		// Camera parameters 
		int camera_num;		 	/* Number of cameras */
		CamParam cameras[4];	/* Cameras parameters */
		// Display
		int disp_height;		/* Display height */
		int disp_width;			/* Display width */
		bool show_debug_img;	/* Debug mode */
		int max_fps;			/* If FPS is higher than max_fps, application sleeps to render at max_fps. 0 is unlimited */
		int msaa;				/* MSAA samples count */
		// Grid parameters
		int grid_angles;		/* Every quadrant of circle will be divided into this number of arcs */
		int grid_start_angle;	/* The parameter sets a circle segment for which the grid will be generated.
								 * It defines 2 points of circle secant. The first point is located in
								 * I quadrant and it is the start point of start_angle arc. And the second point is
								 * located in II quadrant and it is the end point of (grid_angles - grid_start_angle) arc */
		int grid_nop_z;			/* Number of points in z axis */
		float grid_step_x;		/* Step in x axis for bowl side which is used to define grid points in z axis.
 	 	 	 	 	 	 		 * Step in z axis: step_z[i] = (i * step_x_2)^2, i = 1, 2, ... - number of point */
		float bowl_radius;		/* Bowl radius*/
		float smooth_angle;		/* Mask angle of smoothing */
		string keyboard;		/* Keyboard device */
		string mouse;			/* Mouse device */
		string out_disp;		/* Display device */
		float model_scale[3];	/* Car model scale */
	
		/**************************************************************************************************************
		 *
		 * @brief  			Read calibration settings from xml file.
		 *
		 * @param  in		const char* filename - name of xml file with project settings
		 *
		 * @return 			The function returns 0 if all settings were read successfully. Otherwise -1 has been returned.
		 * 					The public properties are set.
		 *
		 * @remarks			The function reeds settings from filename xml file and write them to public properties. 
		 *
		 **************************************************************************************************************/
		int readXML(const char* filename);
		/**************************************************************************************************************
		 *
		 * @brief  			Get template width (maximum value of template vertices x coordinate)
		 *
		 * @param	in		const char* filename - template file name
		 *			out		int* val - pointer to the output value
		 *
		 * @return 			The function returns -1 if template file was not found. Otherwise 0 has been returned.
		 *
		 * @remarks			The function calculates maximum value of template vertices x coordinate and write this value 
		 *					to the *val.
		 *
		 **************************************************************************************************************/
		int getTmpMaxVal(const char* filename, int* val);
		/**************************************************************************************************************
		 *
		 * @brief  			Write all public parameters values
		 *
		 * @param			-	
		 *
		 * @return 			-
		 *
		 * @remarks			The function writes all public parameters values to screen
		 *
		 **************************************************************************************************************/
		void printParam(void);
	private:
		/**************************************************************************************************************
		 *
		 * @brief  			Set public property with val value
		 *
		 * @param	in		int num - property number
		 *			in		const char* val - property new value
		 *
		 * @return 			The function returns 0 if public property was set successfully. Otherwise -1 has been returned.
		 *
		 * @remarks			The function write val value to the public properties with num number. Property can be integer,
		 *					float, string, bool or CamParam structure value.
		 *
		 **************************************************************************************************************/
		int setParam(int num, const char* val);
		/**************************************************************************************************************
		 *
		 * @brief  			Convert and write input string to the bool variable
		 *
		 * @param	in		const char* src - property value
		 *			out		bool* dst - pointer to the bool property
		 *
		 * @return 			-
		 *
		 * @remarks			The function converts and writes input string src to the bool variable *dst.
		 *
		 **************************************************************************************************************/
		void readBool(const char* src, bool* dst);
		/**************************************************************************************************************
		 *
		 * @brief  			Convert and write input string to the integer variable
		 *
		 * @param	in		const char* src - property value
		 *			out		int* dst - pointer to the integer property
		 *
		 * @return 			The function returns 0 if src contains uint value. Otherwise -1 has been returned.
		 *
		 * @remarks			The function converts and writes input string src to the integer variable *dst.
		 *
		 **************************************************************************************************************/
		int readUInt(const char* src, int* dst);
		/**************************************************************************************************************
		 *
		 * @brief  			Convert and write input string to the float variable
		 *
		 * @param	in		const char* src - property value
		 *			out		float* dst - pointer to the float property
		 *
		 * @return 			The function returns 0 if src contains nonnegative float value. Otherwise -1 has been returned.
		 *
		 * @remarks			The function converts and writes input string src to the float variable *dst.
		 *
		 **************************************************************************************************************/
		int readFloat(const char* src, float* dst);
		/**************************************************************************************************************
		 *
		 * @brief  			Convert and write input string to the CamParam structure
		 *
		 * @param	in		const char* src - property value
		 * 					int index - camera index
		 *			out		CamParam* dst - pointer to the CamParam property
		 *
		 * @return 			The function returns -1 if there are too few or too much parameters in the input src string. 
		 *					Otherwise 0 has been returned.
		 *
		 * @remarks			The function converts and writes input string src to the CamParam structure *dst.
		 *
		 **************************************************************************************************************/
		int readCamera(const char* src, int index, CamParam* dst);
		/**************************************************************************************************************
		 *
		 * @brief  			Get parameter number
		 *
		 * @param	in		const char* name - parameter  name
		 *
		 * @return 			int - parameter number.
		 *
		 * @remarks			The function returns parameter number according to the parameter name. If parameter name 
		 *					is not defined then -1 is returned.
		 *
		 **************************************************************************************************************/
		int getParam(const char* name);
};

#endif /* SRC_SETTINGS_HPP_ */
