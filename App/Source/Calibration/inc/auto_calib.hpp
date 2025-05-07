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

#ifndef AUTO_CALIB_HPP_
#define AUTO_CALIB_HPP_

#include "config.hpp"

/**********************************************************************************************************************
 * Types
 **********************************************************************************************************************/
struct camera_view			
{
	int camera_index;
	vector<int> mesh_index;
};

enum viewStates{fisheye_view = 0, defisheye_view = 1, contours_view = 2, grids_view = 3, result_view = 4};


/*******************************************************************************************
 * Global variables
 *******************************************************************************************/
static volatile sig_atomic_t quit = 0;
static viewStates view_state = fisheye_view;

static int tmpl_width;
static int tmpl_height;
static int contours_buf;
static int grid_buf;


static XMLParameters param;			// Cameras parameters
static MyDisplay* disp;				// Display
static View* view;						// View object
static vector<camera_view> cam_views;	// View indexes
static vector<Camera*> cameras;		// Cameras
static vector<CurvilinearGrid*> grids;	// Grids
gst_data gst_shared;					//GStreamer

/*******************************************************************************************
 * Global functions
 *******************************************************************************************/
static int objectsInit(void);
static void objectsFree(void);
static void switchState(viewStates new_state);
static void updateState(viewStates new_state);
static int searchContours(uint index);
static int getContours(float** gl_lines);
static int getGrids(float** gl_grid);
static void saveGrids(void);

#endif /* AUTO_CALIB_HPP_ */
