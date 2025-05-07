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


#ifndef SRC_RENDER_HPP_
#define SRC_RENDER_HPP_



#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>

// String
#include <string>

//OpenGL
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>

//Capturing
#include "src_v4l2.hpp"
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <g2d.h>

//OpenCV
#include "opencv2/highgui/highgui.hpp"
#include <opencv2/imgproc/imgproc.hpp>

//Shaders
#include "gl_shaders.hpp"
#include "shaders.hpp"

/**********************************************************************************************************************
 * Macros
 **********************************************************************************************************************/
#define GL_PIXEL_TYPE GL_RGBA
#define CAM_PIXEL_TYPE V4L2_PIX_FMT_YUYV

/**********************************************************************************************************************
 * Types
 **********************************************************************************************************************/
struct vertices_obj			
{
	GLuint	vao;
	GLuint	vbo;
	GLuint	tex;
	int		num;
};

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
/* Render class */
class View {
public:
	gst_data* gst_shared;			//GStreamer

	View(void);
	~View(void);

	int addProgram(const char* v_shader, const char* f_shader);
	int setProgram(uint index);
	void cleanView(void);
	
	int addCamera(string device, int width, int height);
	int addMesh(string filename);	
	
	void renderView(int camera, int mesh);
	int runCamera(int index);
	
	int createMesh(Mat xmap, Mat ymap, string filename, int density, Point2f top);
	void reloadMesh(int index, string filename);
	int changeMesh(Mat xmap, Mat ymap, int density, Point2f top, int index);
	
	Mat takeFrame(int index);
	
	
	int getVerticesNum(uint num) {if (num < v_obj.size()) { return (v_obj[num].num); } return (-1);}
	
	
	int addBuffer(GLfloat* buf, int num);
	int setBufferAsAttr(int buf_num, int prog_num, const char* atr_name);
	void renderBuffer(int buf_num, int primitive_type, int vert_num);
	void updateBuffer(int buf_num, GLfloat* buf, int num);
	
	
private:
	int current_prog;	
	vector<Programs> render_prog;
	vector<vertices_obj> v_obj;
	vector<v4l2Camera> v4l2_cameras;	// Camera buffers
	
	void vLoad(GLfloat** vert, int* num, string filename);
	void bufferObjectInit(GLuint* text_vao, GLuint* text_vbo, GLfloat* vert, int num);
	void texture2dInit(GLuint* texture);	
};

#endif /* SRC_RENDER_HPP_ */
