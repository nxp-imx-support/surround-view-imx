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


#ifndef DISPLAY_HPP_
#define DISPLAY_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

//EGL
#include <EGL/egl.h>

using namespace std;

/*******************************************************************************************
 * Macros
 *******************************************************************************************/
#define SIGN(x) (x < 0) ? -1 : 1

/*******************************************************************************************
 * Types
 *******************************************************************************************/
// Type of input event
enum ev_type { m_move, m_scroll_up, m_scroll_down, k_esc, k_up, k_down, k_right, k_left, k_f1, k_f5, k_p, ev_none };

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
class MyDisplay {
public:
	/**************************************************************************************************************
	 *
	 * @brief  			MyDisplay class constructor.
	 *
	 * @param  in 		const int width - window width
	 *					const int height - window height
	 *					const char* keyboard_dev - keyboard device file from /dev/input/by-path folder
	 *					const char* mouse_dev - mouse device file from /dev/input/by-path folder
	 *					const int msaa - MSAA samples count
	 *
	 * @return 			The function creates the MyDisplay object.
	 *
	 * @remarks 		The function creates eglNativeDisplayType display. 
	 *					If X11 window system is used then Display* display is created. 
	 *					In this case keyboard and mouse devices ignored because X11 server is used for event catching. 
	 *					If FB backend is used then /dev/fb0 is used for rendering.
	 *					In this case it is necessary to set correct keyboard and mouse devices for event catching.
	 *
	 **************************************************************************************************************/
	MyDisplay(const int width, const int height, const char* keyboard_dev, const char* mouse_dev, const int msaa);

	/**************************************************************************************************************
	 *
	 * @brief  			MyDisplay class destructor.
	 *
	 * @param			-
	 *
	 * @return 			-
	 *
	 * @remarks 		The function sets free all egl objects and closes display
	 *
	 **************************************************************************************************************/
	~MyDisplay(void);

	/**************************************************************************************************************
	 *
	 * @brief  			post EGL surface color buffer to a native window
	 *
	 * @param   		-
	 *
	 * @return 			-
	 *
	 * @remarks 		the function posts surface color buffer to the associated native window. It performs an 
	 *					implicit glFlush before returning. 
	 *
	 **************************************************************************************************************/
	void swapBuffers(void);

	/**************************************************************************************************************
	 *
	 * @brief  			Get number of unprocessed events for x11 or flag of an input event for fb backend
	 *
	 * @param   		-
	 *
	 * @return 			int		The number of unprocessed events for x11 backend
	 *							The flag of an input event for fb backend 
	 *							(1 - there is at least one unprocessed event, 0 - there isn't any unprocessed event)
	 *
	 * @remarks 		The function returns the number of unprocessed events for x11 backend or the flag of an 
	 *					input event for fb backend. The function checks events from keyboard and mouse devices.
	 *					If there isn't any unprocessed event, the function returns 0.
	 *
	 **************************************************************************************************************/
	int getEventsNum(void);
	/**************************************************************************************************************
	 *
	 * @brief  			Get code of current captured event
	 *
	 * @param   		-
	 *
	 * @return 			ev_type		code of current captured event: m_move - mouse moving + mouse left button pressing
	 *																m_scroll_up - mous up scrolling
	 *																m_scroll_down - mouse down scrolling
	 *																k_esc - Esc key pressing
	 *																k_up - Up key pressing
	 *																k_down - Down key pressing
	 *																k_right - Right key pressing
	 *																k_left - Left key pressing
	 *																k_f1 - F1 key pressing
 	 *																k_f5 - F5 key pressing
	 *																ev_none - other non classificate events
	 *
	 * @remarks 		The function returns checks current event description and returns event type.
	 *
	 **************************************************************************************************************/
	ev_type getNextEvent(void);
	float mouse_offset[2] = {0.0, 0.0};							// Current offset for mouse moving event
	
	EGLDisplay getEGLDispaly();

	EGLContext getEGLContext();

private:
	int fd_m = 0;										// Mouse device. Is used only for FB version
	int fd_k = 0;										// Keyboard device. Is used only for FB version
	struct input_event inevent = {};	// Input Event. Is used only for FB version
	bool btn_mouse_left = false;							// Mouse left button click. Is used only for FB version

	float mouse_pos[2] = {0.0, 0.0}; 				// Mouse position. Is used only for X11 version
	
	EGLDisplay				egldisplay = NULL;
	EGLConfig				eglconfig = NULL;
	EGLSurface				eglsurface = NULL;
	EGLContext				eglcontext = NULL;
	EGLNativeDisplayType			eglNativeDisplayType = NULL;
	
	/**************************************************************************************************************
	 *
	 * @brief  			Native display and window initalization
	 *
	 * @param  in 		const int width - window width
	 *					const int height - window height
	 *					const int msaa - MSAA samples count
	 *
	 * @return 			-
	 *
	 * @remarks 		The function creates eglNativeDisplayType display. 
	 *					If X11 window system is used then Display* display is created. 
	 *					In this case keyboard and mouse devices ignored because X11 server is used for event catching. 
	 *					If FB backend is used then /dev/fb0 is used for rendering.
	 *
	 **************************************************************************************************************/
	void dispInit(const int width, const int height, const int msaa);
};

#endif  // GAIN_HPP_
