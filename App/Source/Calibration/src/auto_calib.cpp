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


#include "auto_calib.hpp"

using namespace std;
using namespace cv;

/***************************************************************************************
***************************************************************************************/
static void printState(viewStates vstate)
{
	switch (vstate)
	{				
	case fisheye_view:
		cout << "STEP 0: Load fisheye camera view..." << endl;	
		break;				
	case defisheye_view:
		cout << "STEP 1: Remove fisheye distortion..." << endl;
		break;		
	case contours_view:
		cout << "STEP 2: Search contours..." << endl;
		break;
	case grids_view:
		cout << "STEP 3: Prepare meshes..." << endl;
		break;		
	case result_view:
		cout << "STEP 4: Calculate the result view..." << endl;	
		break;		
	default:
		cout << "ERROR: Unknown state" << endl;
		break;
	}
}

/***************************************************************************************
***************************************************************************************/
static void sig_handler(int sig_code)
{
	cout << "Caught signal " << sig_code << ", setting flaq to quit" << endl;
	quit = 1;
}

/***************************************************************************************
***************************************************************************************/
// Program entry.
int main(int argc, char** argv)
{ 
	if(signal(SIGINT, sig_handler) == SIG_ERR) { 		// ctrl + C
		cout << "ERROR: Error setting up signal handlers"<< endl;
	}
	if(signal(SIGTERM, sig_handler) == SIG_ERR){ 		// kill command
		cout << "ERROR: Error setting up signal handlers" << endl;
	}

	///////////////////// Initialization /////////////////////////
	if (objectsInit() == -1) { return (-1); }
	
	/////////////////////// Rendering ///////////////////////////
	while (quit == 0) // rendering loop
	{
		while (disp->getEventsNum() > 0)
		{
			switch (disp->getNextEvent())
			{				
				// key pressing
			case k_esc:
				// Quit
				cout << "The exit key was pressed" << endl;
				quit = 1;
				break;
				
			case k_right:
				// Next view
				if (view_state < result_view)
				{
					view_state = (viewStates)((int)view_state + 1);
					switchState(view_state);
				}
				break;
						
			case k_left:
				// Previous view
				if (view_state > fisheye_view)
				{
					view_state = (viewStates)((int)view_state - 1);	
					switchState(view_state);
				}
				break;
				
			case k_f5:
				// Update parameters
				if (param.readXML("../Content/settings.xml") == -1) { break; }
				updateState(view_state);
				break;
				
			default:
				break;
			}
		}			
		view->cleanView();	
		

		if (view_state != result_view)
		{
			if(view->setProgram(0) == 0)
			{
				for (uint i = 0U; i < cam_views.size(); i++) {
					view->renderView(cam_views[i].camera_index, cam_views[i].mesh_index[0U]);
				}
			}
		}

		if (view_state == contours_view)
		{
			if(view->setProgram(1) == 0) {
				view->renderBuffer(contours_buf, 0, view->getVerticesNum((uint)contours_buf));
			}
		}

		if (view_state == grids_view)
		{
			if(view->setProgram(1) == 0) {
				view->renderBuffer(grid_buf, 1, view->getVerticesNum((uint)grid_buf));
			}
		}
		
		if (view_state == result_view)
		{
			if(view->setProgram(2) == 0)
			{
				for (uint i = 0U; i < cam_views.size(); i++)
				{
					view->renderView(cam_views[i].camera_index, cam_views[i].mesh_index[1U]);
					view->renderView(cam_views[i].camera_index, cam_views[i].mesh_index[2U]);
				}
			}
		}
				
		disp->swapBuffers();
		glFinish();
	}

	/////////////////////// Free memory ///////////////////////////
	objectsFree();
	
	return 0;
}

/***************************************************************************************
***************************************************************************************/
int objectsInit(void)
{
	////////////////// Read XML parameters /////////////////////
	if (param.readXML("../Content/settings.xml") == -1) { return (-1); }
		
	if (param.getTmpMaxVal("template_1.txt", &tmpl_width) == -1) { return (-1); }
	if (param.getTmpMaxVal("template_2.txt", &tmpl_height) == -1) { return (-1); }
	
	//////////////////////// Display ////////////////////////////
	disp = new MyDisplay(param.disp_width, param.disp_height, param.keyboard.c_str(), param.mouse.c_str(), 0);

	/////////////////// Create view object //////////////////////
	view = new View;

	/* GST initialization */
	gst_init (NULL,NULL);
	GMainLoop* gst_main_loop = g_main_loop_new (NULL, FALSE);

	gst_shared.loop = gst_main_loop;
	gst_shared.gst_display = gst_gl_display_egl_new_with_egl_display(disp->getEGLDispaly());
	gst_shared.gl_context = gst_gl_context_new_wrapped(GST_GL_DISPLAY(gst_shared.gst_display), (guintptr)disp->getEGLContext(), GST_GL_PLATFORM_EGL, GST_GL_API_GLES2);
	view->gst_shared = &gst_shared;

	if (view->addProgram(s_v_shader, s_f_shader) == -1) { return (-1); }
	if (view->addProgram(s_v_shader_line, s_f_shader_line) == -1) { return (-1); }
	if (view->addProgram(s_v_shader_bowl, s_f_shader_bowl) == -1) { return (-1); }
	if (view->setProgram(0) == -1) { return (-1); }
		
	for (int i = 0; i < param.camera_num; i++)
	{
		camera_view cam_view;
		cam_view.camera_index = view->addCamera(param.cameras[i].device, param.cameras[i].width, param.cameras[i].height);
		if(cam_view.camera_index == -1) { return (-1); }
		cam_view.mesh_index.push_back(view->addMesh(string("../Content/meshes/original/mesh" + to_string(i + 1))));
		cam_views.push_back(cam_view);
	}
	
	for (int i = 0; i < param.camera_num; i++) {
		view->runCamera(i);
	}

	///////////////////// Create camera objects //////////////////
	for (int i = 0; i < param.camera_num; i++) 
	{
		string calib_res_txt = param.camera_models + "/calib_results_" + to_string(i + 1) + ".txt"; // Camera model
		Creator creator;
		Camera* camera = creator.create(calib_res_txt.c_str(), param.cameras[i].sf, i); // Create Camera object
		if (camera == NULL) 
		{
			cout << "ERROR: Failed to create camera model" << endl;
			return (-1);
		}
		cameras.push_back(camera);
	}
	
	///////////////////// Create grid objects //////////////////
	for (int i = 0; i < param.camera_num; i++) 
	{
		CurvilinearGrid* grid_element = new CurvilinearGrid(param.grid_angles, param.grid_start_angle, param.grid_nop_z, param.grid_step_x);
		grids.push_back(grid_element);
	}	
	return 0;
}

/***************************************************************************************
***************************************************************************************/
void objectsFree(void)
{
	delete disp;
	delete view;
	for (int i = (int)cameras.size() - 1; i > 0; i--) {
		delete cameras[i];
	}
	for (int i = (int)grids.size() - 1; i > 0; i--) {
		delete grids[i];		
	}	
}

/***************************************************************************************
***************************************************************************************/
void switchState(viewStates new_state)
{
	float* data;
	int data_num;

	printState(new_state);

	switch (new_state)
	{				
	case fisheye_view:
		// Fisheye
		for (uint i = 0U; i < cameras.size(); i++)
		{
			string filename = "../Content/meshes/original/mesh" + to_string(i + 1U);
			view->reloadMesh(cam_views[i].mesh_index[0U], filename);
		}	
		break;
				
	case defisheye_view:
		// Defisheye
		for (uint i = 0U; i < cameras.size(); i++) {
			view->changeMesh(cameras[i]->xmap, cameras[i]->ymap, 10, Point2f((static_cast<float>(i & 1U) - 1.0f), static_cast<float>((~i >> 1U) & 1U)), cam_views[i].mesh_index[0U]);
		}
		break;
		
	case contours_view:
		// Contours searching
		data_num = getContours(&data);
		contours_buf = view->addBuffer(&data[0], data_num / 3);
		view->setBufferAsAttr(contours_buf, 1, (const char*)"vPosition");
		if (data != NULL) { free(data); }
		break;

	case grids_view:
		// Mesh generation
		data_num = getGrids(&data);
		grid_buf = view->addBuffer(&data[0], data_num / 3);
		view->setBufferAsAttr(grid_buf, 1, (const char*)"vPosition");
		if (data != NULL) { free(data); }
		break;
		
	default: // result_view
		// Result view
		saveGrids();
		for (uint i = 0U; i < cam_views.size(); i++)
		{
			if (cam_views[i].mesh_index.size() == 1)
			{
				cam_views[i].mesh_index.push_back(view->addMesh(string("array" + to_string(i + 1U) + "1")));
				cam_views[i].mesh_index.push_back(view->addMesh(string("array" + to_string(i + 1U) + "2")));
			}
			else
			{
				view->reloadMesh(cam_views[i].mesh_index[1U], string("array" + to_string(i + 1U) + "1"));
				view->reloadMesh(cam_views[i].mesh_index[2U], string("array" + to_string(i + 1U) + "2"));			
			}
		}

		break;		
	}
	cout << "\tDone" << endl;
}


/***************************************************************************************
***************************************************************************************/
void updateState(viewStates new_state)
{
	float* data;
	int data_num;

	printState(new_state);

	switch (new_state)
	{		
		
	case defisheye_view:
		// Defisheye
		for (uint i = 0U; i < cameras.size(); i++) 
		{
			cameras[i]->updateLUT(param.cameras[i].sf);	
			view->changeMesh(cameras[i]->xmap, cameras[i]->ymap, 10, Point2f((static_cast<float>(i & 1U) - 1.0f), static_cast<float>((~i >> 1U) & 1U)), cam_views[i].mesh_index[0U]);
		}
		break;
				
	case contours_view:
		// Contours searching
		data_num = getContours(&data);
		view->updateBuffer(contours_buf, &data[0], data_num / 3);
		if (data != NULL) { free(data); }
		break;
	
	case grids_view:
		// Mesh generation
		data_num = getGrids(&data);
		view->updateBuffer(grid_buf, &data[0], data_num / 3);
		if (data != NULL) { free(data); }
		break;
		
	default: //result_view
		for (uint i = 0U; i < cam_views.size(); i++)
		{
			view->reloadMesh(cam_views[i].mesh_index[1U], string("array" + to_string(i + 1U) + "1"));
			view->reloadMesh(cam_views[i].mesh_index[2U], string("array" + to_string(i + 1U) + "2"));
		}
		break;
	}
	cout << "\tDone" << endl;
}


/***************************************************************************************
***************************************************************************************/
int searchContours(uint index)
{
	Mat img = view->takeFrame(cam_views[index].camera_index);
								
	string ref_points_txt = param.tmplt + "/template_" + to_string(index + 1U) + ".txt"; // Template points
	string chessboard = param.camera_models + "/chessboard_" + to_string(index + 1U) + "/"; // Chessboard image

	// Set roi in which contours will be searched (in % of image height) and empiric bound for minimal allowed perimeter for contours
	cameras[index]->setRoi(param.cameras[index].roi);
	cameras[index]->setContourMinSize(param.cameras[index].cntr_min_size);
	// Set template size and reference points
	if (cameras[index]->setTemplate(ref_points_txt.c_str(), Size2d(tmpl_width, tmpl_height)) != 0) { return (-1); }
	// Calculate intrinsic camera parameters using chessboard image
	string chessboard_name = string("frame" + to_string(index + 1U) + "_");
	if (cameras[index]->setIntrinsic(chessboard.c_str(), chessboard_name.c_str(), param.cameras[index].chessboard_num, Size(7, 7)) != 0) { return (-1); }
	//Estimate extrinsic camera parameters using calibrating template
	if (cameras[index]->setExtrinsic(img) != 0) { return (-1); }
	return (0);
}

/***************************************************************************************
***************************************************************************************/		
int getContours(float** gl_lines)
{
	int sum_num = 0;
	if(cameras.size() > 0U) {
		float** contours = (float**)calloc((size_t)cameras.size(), sizeof(float*)); // Contour arrays for each camera
		if (contours == NULL) {
			cout << "ERROR: Cannot allocate memory for template contours" << endl;
			return(0);
		}

		int* array_num = (int*)calloc((size_t)cameras.size(), sizeof(int)); // Number of array elements for each camera
		if (array_num == NULL) {
			cout << "ERROR: Cannot allocate memory for template contours" << endl;
			free(contours);
			return(0);
		}
		int index = 0;
	
		for (uint i = 0U; i < cameras.size(); i++)
		{
			if(searchContours(i) == 0)
			{
				array_num[i] = cameras[i]->getContours(&contours[i]);
				sum_num += array_num[i];
			}
		}
		*gl_lines = new float[sum_num];
		for (uint i = 0U; i < cameras.size(); i++)
		{
			for (int j = 0; j < array_num[i]; j++)
			{
				(*gl_lines)[index] = contours[i][j];
				index++;				
			}
		}		
		for (int i = (int)cameras.size() - 1; i >= 0; i--) {
			if (contours[i] != NULL) { free(contours[i]); }
		}
		free(contours);
		free(array_num);
	}
	return sum_num;	
}
	

/***************************************************************************************
***************************************************************************************/	
int getGrids(float** gl_grid)
{
	int sum_num = 0;
	if(cameras.size() > 0U) { 
		float** grids_data = (float**)calloc((size_t)cameras.size(), sizeof(float*)); // Contour arrays for each camera
		if (grids_data == NULL) 
		{
			cout << "ERROR: Cannot allocate memory for 3D grids" << endl;
			return(0);
		}

		int* array_num = (int*)calloc((size_t)cameras.size(), sizeof(int));	// Number of array elements for each camera
		if (array_num == NULL) 
		{
			cout << "ERROR: Cannot allocate memory for 3D grids" << endl;
			free(grids_data);
			return(0);
		}
		int index = 0;

		int nopz = param.grid_nop_z;
		for (uint i = 0U; i < cameras.size(); i++) { 	// Get number of points in z axis
			int tmp = cameras[i]->getBowlHeight(param.bowl_radius * cameras[i]->getBaseRadius(), param.grid_step_x);
			nopz = MIN(nopz, tmp);
		}

		vector< vector<Point3f> > seam;
		for (uint i = 0U; i < grids.size(); i++) {
			*grids[i] = CurvilinearGrid(param.grid_angles, param.grid_start_angle, nopz, param.grid_step_x);
			if(grids[i] == NULL) { continue; }	
			grids[i]->createGrid(cameras[i], param.bowl_radius * cameras[i]->getBaseRadius()); // Calculate grid points and save grid to the file
			array_num[i] = grids[i]->getGrid(&grids_data[i]);
			sum_num += array_num[i];
		}

		*gl_grid = new float[sum_num];
		for (uint i = 0U; i < cameras.size(); i++)
		{
			for (int j = 0; j < array_num[i]; j++)
			{
				(*gl_grid)[index] = grids_data[i][j];
				index++;				
			}
		}		

		for (int i = (int)cameras.size() - 1; i >= 0; i--) {
			if (grids_data[i] != NULL) { free(grids_data[i]); }
		}
		free(grids_data);
		free(array_num);
	}

	return sum_num;
}



/***************************************************************************************
***************************************************************************************/	
void saveGrids(void)
{
	vector< vector<Point3f> > seams;
	vector<Point3f> seam_points;		
	for (uint i = 0U; i < grids.size(); i++) 
	{
		grids[i]->saveGrid(cameras[i]);
		grids[i]->getSeamPoints(seam_points);	// Get grid seams
		seams.push_back(seam_points);
	}

	Masks masks;
	masks.createMasks(cameras, seams, param.smooth_angle); // Calculate masks for blending       
	if(masks.splitGrids() == -1) {
		cout << "ERROR: Texels/vertices grids have not been split" << endl;
	}

	Compensator compensator(Size(param.disp_width, param.disp_height)); // Exposure correction  
	compensator.feed(cameras, seams);
	if(compensator.save((const char*)"./compensator") == -1) {
		cout << "ERROR: Compensator grids have not been saved" << endl;
	}
}
