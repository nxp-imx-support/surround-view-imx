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

#ifndef CAPTURING_HPP_
#define CAPTURING_HPP_

/**********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

//Threads
#include <signal.h>

// String
#include <string>

//OpenGL
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

//OpenCV
#include <opencv2/highgui/highgui.hpp>

//XML settings
#include "settings.hpp"

//Rendering
#include "view.hpp"

//EGL
#include "display.hpp"


/**********************************************************************************************************************
 * Types
 **********************************************************************************************************************/
struct camera_view			
{
	int camera_index;
	vector<int> mesh_index;
};

/*******************************************************************************************
 * Global variables
 *******************************************************************************************/
static volatile sig_atomic_t quit = 0;

static XMLParameters param;			// Cameras parameters
static MyDisplay* disp;				// Display
static View* view;						// View object
static camera_view cam_view;	// View indexes
gst_data gst_shared;			//GStreamer

/*******************************************************************************************
 * Global functions
 *******************************************************************************************/
static int objectsInit(int camera_num);
static void objectsFree(void);

#endif /* CAPTURING_HPP_ */
