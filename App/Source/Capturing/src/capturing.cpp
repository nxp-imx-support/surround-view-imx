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


#include "capturing.hpp"

using namespace std;
using namespace cv;

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
	int frame_num = 0;

	if(signal(SIGINT, sig_handler) == SIG_ERR) { 		// ctrl + C
		cout << "Error setting up signal handlers"<< endl;
	}
	if(signal(SIGTERM, sig_handler) == SIG_ERR){ 		// kill command
		cout << "Error setting up signal handlers" << endl;
	}

	///////////////////// Initialization /////////////////////////
	int camera_num = 0;
	if (argc > 1) {
		camera_num = atoi(argv[1]) - 1;
		if ((camera_num < 0) || (camera_num > 3)) {
			cout << "Camera numbers must be in [1, 4]" << endl;
			camera_num = 0;
		}
		cout << "Camera " << camera_num + 1 << " will be opened" << endl;
	}

	if (objectsInit(camera_num) == -1) { return (-1); }

	/////////////////////// Rendering ///////////////////////////
	while (quit == 0) // rendering loop
	{
		while ((disp->getEventsNum() > 0))
		{
			ev_type ev_key = disp->getNextEvent();
			if(ev_key == k_esc)
			{
				cout << "The exit key was pressed" << endl;
				quit = 1;
			}
			else {
				if(ev_key == k_p)
				{
					string frame_name = string("frame" + to_string(camera_num + 1) + "_" + to_string(frame_num) + ".jpg");
					if (imwrite(frame_name.c_str(), view->takeFrame(0)) == false) {
						cout << "Failed to save the image " << frame_name.c_str() << endl;
						continue;
					}
					else {
						frame_num++;
					}
				}
			}
		}

		view->cleanView();	
		view->renderView(cam_view.camera_index, (int)(cam_view.mesh_index[0]));
		glFinish();
				
		disp->swapBuffers();
	}

	/////////////////////// Free memory ///////////////////////////
	g_main_loop_quit(gst_shared.loop);
	objectsFree();
	
	return 0;
}

/***************************************************************************************
***************************************************************************************/
int objectsInit(int camera_num)
{
	////////////////// Read XML parameters /////////////////////
	if (param.readXML("../../App/Content/settings.xml") == -1) { return (-1); }
	//param.printParam();

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
	if (view->setProgram(0) == -1) { return (-1); }

	cam_view.camera_index = view->addCamera(param.cameras[camera_num].device, param.cameras[camera_num].width, param.cameras[camera_num].height);
	if(cam_view.camera_index == -1) { return (-1); }
	cam_view.mesh_index.push_back(view->addMesh(string("../../App/Content/meshes/fullscrean")));

	if (view->runCamera(0) == -1) { return (-1); }

	return 0;
}

/***************************************************************************************
***************************************************************************************/
void objectsFree(void)
{
	delete disp;
	delete view;
}
