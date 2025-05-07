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


#include "settings.hpp"

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
int XMLParameters::readXML(const char* filename)
{
	xmlDocPtr pdoc;
	xmlNodePtr pnode;

	pdoc = xmlReadFile(filename, NULL, 0);

	if (pdoc == NULL) {
		cout << "Cannot open the " << filename << " file" << endl;
		return (-1);
	}

	pnode = xmlDocGetRootElement(pdoc);

	if (pnode == NULL) {
		cout << "The " << filename << " document is empty" << endl;
		xmlFreeDoc(pdoc);
		return (-1);
	}
	
	for (pnode = pnode->children; pnode != NULL; pnode = pnode->next) {
		if (pnode->type == XML_ELEMENT_NODE) {
			xmlNodePtr pchildren  = pnode->xmlChildrenNode;
			for (pchildren = pnode->children; pchildren != NULL; pchildren = pchildren->next) {
				if (pchildren->type == XML_ELEMENT_NODE) {
					xmlChar* ret_val = xmlNodeGetContent(pchildren);
					if(ret_val != NULL){
						if (setParam(getParam((const char*)pchildren->name), (const char*)ret_val) == -1) {
							xmlFreeDoc(pdoc);
							return (-1);
						}
					}
				}
			}
		}
	}
	xmlFreeDoc(pdoc);
	return (0);
}

/**************************************************************************************************************
 *
 * @brief  			Set public property with val value
 *
 * @param	in		int num - property number
 *			in		const char* val - property new value
 *
 * @return 			The function returns 0 if public property was set successfully. Otherwise -1 has been returned.
 *
 * @remarks			The function write val value to the public properties with num number. Property can be
 *					unsigned integer, float, string, bool or CamParam structure value.
 *
 **************************************************************************************************************/
int XMLParameters::setParam(int num, const char* val)
{
	int ret_val = 0;
	switch (num)
	{
		case 0: // char* camera_inputs;
			camera_inputs = string(val);
			break;
		case 1: // char* camera_models;
			camera_models = string(val);
			break;
		case 2: // char* tmplt;
			tmplt = string(val);
			break;
		case 3: // int camera_num;
			camera_num = atoi(val);
			if ((camera_num < 1) || (camera_num > 4)) {
				cout << "Camera numbers must be in [1, 4]" << endl;
				return (-1);
			}
			break;
		case 4: // int disp_height
			ret_val = readUInt(val, &disp_height);
			break;
		case 5: // int disp_width
			ret_val = readUInt(val, &disp_width);
			break;		
		case 6: // bool show_debug_img;
			readBool(val, &show_debug_img);
			break;			
		case 7: // int grid_angles;
			ret_val = readUInt(val, &grid_angles);
			break;	
		case 8: // int grid_start_angle;
			ret_val = readUInt(val, &grid_start_angle);
			break;		
		case 9: // int grid_nop_z;
			ret_val = readUInt(val, &grid_nop_z);
			break;		
		case 10: // float grid_step_x;
			ret_val = readFloat(val, &grid_step_x);
			break;
		case 11: // float bowl_radius;
			ret_val = readFloat(val, &bowl_radius);
			break;
		case 12: // float smooth_angle;
			ret_val = readFloat(val, &smooth_angle);
			break;
		case 13: // char* keyboard;
			keyboard = string(val);
			break;	
		case 14: // char* mouse;
			mouse = string(val);
			break;	
		case 15: // char* out_disp;
			out_disp = string(val);
			break;	
		case 16: // float model_scale[3];
			ret_val = readFloat(val, &model_scale[0]);
			break;	
		case 17: // float model_scale[3];
			ret_val = readFloat(val, &model_scale[1]);
			break;	
		case 18: // float model_scale[3];
			ret_val = readFloat(val, &model_scale[2]);
			break;	
		case 19: // int max_fps;
			ret_val = readUInt(val, &max_fps);
			break;
		case 20: // int msaa;
			ret_val = readUInt(val, &msaa);
			break;
		case 100: case 101: case 102: case 103: // CamParam cameras[4];
			ret_val = readCamera(val, num - 100, &cameras[num - 100]);
			break;		
		default:
			cout << "Too much parameters in xml file" << endl;
			ret_val = -1;
			break;
	}	
	return (ret_val);
}

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
int XMLParameters::readUInt(const char* src, int* dst)
{
	*dst = atoi(src);
	if (*dst < 0) {
		cout << "All parameters must be a positive number" << endl;
		return (-1);
	}
	return (0);
}

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
int XMLParameters::readFloat(const char* src, float* dst)
{
	*dst = (float)atof(src);
	if (*dst < 0.0) {
		cout << "All parameters must be a positive number" << endl;
		return (-1);
	}
	return (0);
}

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
void XMLParameters::readBool(const char* src, bool* dst)
{
	int intval = atoi(src);
	*dst = (intval != 0);
}

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
int XMLParameters::readCamera(const char* src, int index, CamParam* dst)
{
	float buf[6] = {0.0f};
	stringstream str; 
	str << src;

	// Read 6 first decimal parameters
	for (int i = 0; i < 6; ++i)
	{
		if (!(str >> buf[i]))
		{
			cout << "Too few parameters for a camera object" << endl;
			return (-1);
		}
	}
	dst->height = (int)buf[0];
	dst->width = (int)buf[1];
	dst->sf = buf[2];
	dst->roi = (int)buf[3];
	dst->cntr_min_size = (int)buf[4];
	dst->chessboard_num = (int)buf[5];

	// Read last string parameter
#ifdef CAMERAS
	if (!(str >> dst->device))
	{
		cout << "<device> parameter not set in camera settings. Setting default value: /dev/video" << index << endl;
		dst->device = "/dev/video" + to_string(index);
	}
#else
	dst->device = "../Content/camera_inputs/src_" + to_string(index + 1);
	//dst->device = "/home/root/SurroundView/App/Content/camera_inputs/src_" + to_string(index + 1);
#endif

	return (0);
}

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
void XMLParameters::printParam(void)
{
	cout << "Path to camera calibration static images " << camera_inputs << endl;
	cout << "Path to camera models " << camera_models << endl;
	cout << "Path to templates " << tmplt << endl;
	cout << "Number of cameras " << camera_num << endl;
	for (int i = 0; i < 4; i++)
	{
		cout << "Camera " << i + 1 << endl;
		cout << "\tResolution " << cameras[i].height << " x " << cameras[i].width  << endl;
		cout << "\tDefisheye scale factor sf = " << cameras[i].sf << endl;
		cout << "\tROI in which contours will be searched (% of image height) roi = " << cameras[i].roi << endl;
		cout << "\tContours min size = " << cameras[i].cntr_min_size << endl;
		cout << "\tChessboard images number = " << cameras[i].chessboard_num << endl;
		cout << "\tDevice = " << cameras[i].device << endl;
	}
	cout << "Display resolution " << disp_height << " x " << disp_width << endl;
	cout << "Show debug info " << show_debug_img << endl;
	cout << "Max FPS = " << max_fps << endl;
	cout << "MSAA samples count = " << msaa << endl;
	cout << "Grig parameters" << endl;
	cout << "\tAngles number " << grid_angles << endl;
	cout << "\tStart angle " << grid_start_angle << endl;
	cout << "\tNumber of grid points in z axis " << grid_nop_z << endl;
	cout << "\tStep in x axis " << grid_step_x << endl;
	cout << "\tRadius of 3D bowl " << bowl_radius << endl;
	cout << "Mask angle of smoothing " << smooth_angle << endl;
	cout << "Keyboard events " << keyboard << endl;
	cout << "Mouse events " << mouse << endl;
	cout << "Display file " << out_disp << endl;
	cout << "Car model scale (x,y,z) " << model_scale[0] << ", " << model_scale[0] << ", " << model_scale[0] << endl;
}

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
int XMLParameters::getTmpMaxVal(const char* filename, int* val)
{
	*val = 0;
	struct stat st;
	string ref_points_txt = tmplt + "/" + (string)filename;
	if (stat(ref_points_txt.c_str(), &st) != 0)
	{
		cout << "File " << ref_points_txt << " not found" << endl;
		return (-1);
	}
	int x, y;
	ifstream ifs_ref(ref_points_txt.c_str());
	while (ifs_ref >> x >> y) {
		if (x > *val) { *val = x; }
	}
	ifs_ref.close();
	return (0);
}

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
int XMLParameters::getParam(const char* name)
{
	int return_val = -1;
	if (strcmp(name, "camera_inputs") == 0) { return_val = 0; }
	else if (strcmp(name, "camera_models") == 0) { return_val = 1; }
	else if (strcmp(name, "template") == 0) { return_val = 2; }
	else if (strcmp(name, "number") == 0) { return_val = 3; }
	else if (strcmp(name, "height") == 0) { return_val = 4; }
	else if (strcmp(name, "width") == 0) { return_val = 5; }
	else if (strcmp(name, "show_debug_img") == 0) { return_val = 6; }
	else if (strcmp(name, "angles") == 0) { return_val = 7; }
	else if (strcmp(name, "start_angle") == 0) { return_val = 8; }
	else if (strcmp(name, "nop_z") == 0) { return_val = 9; }
	else if (strcmp(name, "step_x") == 0) { return_val = 10; }
	else if (strcmp(name, "radius") == 0) { return_val = 11; }
	else if (strcmp(name, "smooth_angle") == 0) { return_val = 12; }
	else if (strcmp(name, "keyboard") == 0) { return_val = 13; }
	else if (strcmp(name, "mouse") == 0) { return_val = 14; }
	else if (strcmp(name, "display") == 0) { return_val = 15; }
	else if (strcmp(name, "x_scale") == 0) { return_val = 16; }
	else if (strcmp(name, "y_scale") == 0) { return_val = 17; }
	else if (strcmp(name, "z_scale") == 0) { return_val = 18; }
	else if (strcmp(name, "max_fps") == 0) { return_val = 19; }
	else if (strcmp(name, "msaa") == 0) { return_val = 20; }
	else if (strcmp(name, "camera1") == 0) { return_val = 100; }
	else if (strcmp(name, "camera2") == 0) { return_val = 101; }
	else if (strcmp(name, "camera3") == 0) { return_val = 102; }
	else if (strcmp(name, "camera4") == 0) { return_val = 103; }
	else { return_val = -1; }
	return (return_val);		
}
