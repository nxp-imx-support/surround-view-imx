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

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1

#include <fstream>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// String
#include <string>

//Threads
#include <signal.h>

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

//Exposure correction
#include "gain.hpp"

//Camera movement
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//Shaders
#include "gl_shaders.hpp"
#include "shaders.hpp"

//Miscellaneous
#include "ModelLoader/ModelLoader.hpp"
#include "MRT.hpp"
#include "FontRenderer.hpp"

//XML settings
#include "settings.hpp"

//Macros
#include "macros.hpp"

//EGL
#include "display.hpp"

/*******************************************************************************************
 * Macros
 *******************************************************************************************/
#define CAMERA_NUM  4
#define VAO_NUM 8 // 2 * CAMERA_NUM

/* GL_VIV_direct_texture */
#ifndef GL_VIV_direct_texture
#define GL_VIV_YV12                     0x8FC0
#define GL_VIV_NV12                     0x8FC1
#define GL_VIV_YUY2                     0x8FC2
#define GL_VIV_UYVY                     0x8FC3
#define GL_VIV_NV21                     0x8FC4
#endif

#endif

