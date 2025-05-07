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

#ifndef CAMERA_TEX_HPP_
#define CAMERA_TEX_HPP_

#include "config.hpp"

/*******************************************************************************************
 * Global variables
 *******************************************************************************************/

static volatile sig_atomic_t quit = 0;
static int expcor = 0;
static double fpsValue = 0.0;

//Cameras parameters
static XMLParameters param;
static int camera_num;						// Cameras number
static vector<int> g_in_width;				// Input frame width
static vector<int> g_in_height;			// Input frame height
static vector<v4l2Camera> v4l2_cameras;	// Camera buffers

// Cameras mapping
static GLuint gTexObj[VAO_NUM] = {0};		// Camera textures
static GLuint txtMask[CAMERA_NUM] = {0};	// Camera masks textures
static vector<Mat> masks;			// Mask images

#define GL_PIXEL_TYPE GL_VIV_UYVY
#define CAM_PIXEL_TYPE V4L2_PIX_FMT_UYVY

static GLuint VAO[VAO_NUM];
static vector<int> vertices;						   

static Programs renderProgram;
static Programs renderProgramWB;

// Exposure correction
static GLuint VAO_EC[CAMERA_NUM];
static vector<int> vertices_ec;

static GLint viewport[4];
static GLuint fbo, rbo;
static Programs exposureCorrectionProgram;
static GLint locGain[2];
static Gains* gain = NULL;

//Model Loader
static glm::mat4 gProjection;	// Initialization is needed
static float rx = 0.0f, ry = 0.0f, px = 0.0f, py = 0.0f, pz = -10.0f;
static glm::vec3 car_scale = glm::vec3(1.0f, 1.0f, 1.0f);

#define CAR_ORIENTATION_X 90.0f 
#define CAR_ORIENTATION_Y 270.0f
#define CAM_LIMIT_RY_MIN -1.57f
#define CAM_LIMIT_RY_MAX 0.0f
#define CAM_LIMIT_ZOOM_MIN -11.5f
#define CAM_LIMIT_ZOOM_MAX -2.5f

static ModelLoader modelLoader;
static Programs carModelProgram;
static GLint mvpUniform, mvUniform, mnUniform;

//MRT
static MRT* mrt = NULL;	// Initialization is needed
static Programs showTexProgram;

//Font Renderer
static FontRenderer* fontRenderer = NULL;	// Initialization is needed
static Programs fontProgram;

//Display
static MyDisplay* out_disp = NULL; // Initialization is needed 

//GStreamer
gst_data gst_shared;


/*******************************************************************************************
 * Global functions
 *******************************************************************************************/
// Get fps rate
static double report_fps(void);

static int programsInit(void);
static void programsDestroj(void);
static int setParam(XMLParameters* xml_param);
static void texture2dInit(GLuint* texture);
static void bufferObjectInit(GLuint* text_vao, GLuint* text_vbo, GLfloat* vert, int num);
static void vLoad(GLfloat** vert, int* num, string filename);
static int camerasInit(void);
static void camTexInit(void);
static void ecTexInit(void);
static inline void mapFrame(int buf_index, int camera);
	
#endif /* CAMERA_TEX_HPP_ */

