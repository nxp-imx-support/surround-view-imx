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

#ifndef SRC_V4L2_HPP_
#define SRC_V4L2_HPP_

/*****************************************************************************************************************
 * Includes
 *****************************************************************************************************************/
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio/videoio.hpp>

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/gl/egl/gstgldisplay_egl.h>
#include <gst/gl/egl/gsteglimage.h>

using namespace cv;
using namespace std;
/**********************************************************************************************************************
 * Macros
 **********************************************************************************************************************/
#define BUFFER_NUM		4	// number of capture buffers

/**********************************************************************************************************************
 * Types
 **********************************************************************************************************************/
struct videobuffer			// Buffer structure
{
	Mat frame;				// Has been used only for video inputs
	unsigned char *start = NULL;
	uint8_t width;
	uint8_t height;
	size_t offset;	
	unsigned int buf_length;
	int filled;		// 1: filled with camera data, 0:camera data has been glTexDirectMapped
	int invalid;	// 1: already swapped, 0:not swappedto texture
};

typedef struct _gst_data
{
	GMainLoop* loop;
	GstGLDisplayEGL* gst_display;
    GstGLContext* gl_context;
	GstElement* pipeline;
	guint bus_watch_id;

} gst_data;



struct thread_arg
{
#ifdef VIDEOS
	int frame_num;
	string device;			// device
	int* fill_buffer_inx;	// 1: filled with camera data, 0: not filled
	videobuffer* buffers[BUFFER_NUM];
#elif defined(RAW)
	FILE *fp;				// Video file  
	int* fill_buffer_inx;	// 1: filled with camera data, 0: not filled
	videobuffer* buffers[BUFFER_NUM];
#else
	int* fd;				// device  
	int* fill_buffer_inx;	// 1: filled with camera data, 0: not filled
	v4l2_buffer* capture_buf[BUFFER_NUM];
	videobuffer* buffers[BUFFER_NUM];
#endif
};


/**********************************************************************************************************************
 * Classes
 **********************************************************************************************************************/
/* v4l2Camera class */
class v4l2Camera {
	public:
		int fill_buffer_inx;	// 1: buffer is filled with camera data, 0: not filled
		videobuffer buffers[BUFFER_NUM]; // buffers
		static pthread_mutex_t th_mutex;	// Mutex for camera access sinchronization
		static int exit_flag;				// Exit flag

		//gst_data gst_context;
		GstElement* gst_pipeline;
		GstGLDisplayEGL* gst_display;
    	GstGLContext* gst_gl_context;
		GMainLoop* gst_loop;
		GMutex frame_lock;
		GstBuffer* buffer_last;
    	GstBuffer* buffer_render;

		int camera_num = 0;

		int getWidth(void) {return width;}		// Camera frame width
		int getHeight(void) {return height;}	// Camera frame height
	
		/**************************************************************************************************************
		 *
		 * @brief  			v4l2Camera class constructor.
		 *
		 * @param  in 		int in_width - input frame width 
		 *					int in_height - input frame height
		 *					int in_pixel_fmt - camera pixel format
		 *					int in_mem_type - memory type
		 *					const char* in_device - camera device name
		 *
		 * @return 			The function creates the v4l2Camera object.
		 *
		 * @remarks 		The function creates v4l2Camera object and initializes object attributes.
		 *
		 **************************************************************************************************************/
		v4l2Camera(int in_width, int in_height, int in_pixel_fmt, int in_mem_type, const char* in_device);

		/**************************************************************************************************************
		 *
		 * @brief  			v4l2Camera class destructor.
		 *
		 * @param  in 		-
		 *
		 * @return 			The function deletes the v4l2Camera object.
		 *
		 * @remarks 		The function deletes v4l2Camera object and cleans object attributes.
		 *
		 **************************************************************************************************************/
		~v4l2Camera(void);

		/**************************************************************************************************************
		 *
		 * @brief  			Set capturing settings
		 *
		 * @param   		-
		 *
		 * @return 			The function returns 0 if all settings were set successfully. Otherwise -1 has been returned.
		 *
		 * @remarks 		The function opens camera devices and sets capturing mode.
		 *
		 **************************************************************************************************************/
		int captureSetup(gst_data* gst_shared);
		/**************************************************************************************************************
		 *
		 * @brief  			Start camera capturing
		 *
		 * @param			-
		 *
		 * @return 			The function returns 0 if capturing was run successfully. Otherwise -1 has been returned.
		 *
		 * @remarks			The function starts camera capturing
		 *
		 **************************************************************************************************************/
		int startCapturing(void);
		/**************************************************************************************************************
		 *
		 * @brief  			Stop camera capturing
		 *
		 * @param			-
		 *
		 * @return 			-
		 *
		 * @remarks			The function stop camera capturing, releases all memory and close camera device.
		 *
		 **************************************************************************************************************/
		void stopCapturing(void);
		/**************************************************************************************************************
		 *
		 * @brief  			Create thread with camera frame capturing loop
		 *
		 * @param   		-
		 *
		 * @return 			The function returns 0 if capturing thread was created successfully.
		 *
		 * @remarks 		The function creates thread with camera frame capturing loop.
		 *					
		 *					The function isn't used for image inputs. It always returns 0.
		 *
		 **************************************************************************************************************/
		int getFrame(void);	
	private:

		FILE *fp = NULL;	// RAW video sources (has been used only for raw video inputs)
		int frame_num = 0;	// Number of frame for video input
		int width;		// Camera frame width
		int height;		// Camera frame height
		int pixel_fmt;		// Camera pixel format
		int mem_type;		// Memory type
		string device;		// Camera device name
		int fd = -1;				// Camera device id
		thread_arg th_arg;	// Thread arguments
		pthread_t get_frame_th = 0;		// Capturing thread
		v4l2_buffer capture_buf[BUFFER_NUM] = {};	// Capturing buffers
		/**************************************************************************************************************
		 *
		 * @brief  			Capturing thread
		 *
		 * @param   in		void* input_args (thread_arg).
		 *									Video inputs:	FILE *fp;	- Video file  
		 *													int* fill_buffer_inx;	- 1: filled with camera data, 0: not filled
		 *													videobuffer* buffers[BUFFER_NUM];	- Pointer to videobuffer structures
		 *
		 *									Camera inputs:	int* fd;	- Thread id
		 *													int* fill_buffer_inx;	- Index of buffer which contain last captured camera frame
		 *													v4l2_buffer* capture_buf[BUFFER_NUM];	- Pointer to captured buffer
		 *													videobuffer* buffers[BUFFER_NUM];		- Pointer to videobuffer structures
		 *
		 * @return 			-
		 *
		 * @remarks 		The function creates thread with camera frame capturing loop. The capturing loop is terminated
		 *					when the exit flag exit_flag is set to 1. Access to camera is synchronized by the mutex th_mutex.
		 *					
		 *					The function isn't used for image inputs.
		 *
		 **************************************************************************************************************/
		static void* getFrameThread(void* input_args);
		static GstFlowReturn OnNewSample(GstElement* appsink, gpointer data);
		static GstPadProbeReturn OnQuery(GstPad* pad, GstPadProbeInfo* info, gpointer data);
};


#endif /* SRC_V4L2_HPP_ */
