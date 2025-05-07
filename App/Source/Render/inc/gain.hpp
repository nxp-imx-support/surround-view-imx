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

#ifndef GAIN_HPP_
#define GAIN_HPP_

/*******************************************************************************************
 * Includes
 *******************************************************************************************/
//Threads
#include <pthread.h>
#include <semaphore.h>

//OpenCV
#include "opencv2/highgui/highgui.hpp"

//Exposure correction
#include "exposure_compensator.hpp"

//Macros
#include "macros.hpp"

#define CHANELS_NUM 3
#define CAMERAS_NUM 4

/*******************************************************************************************
 * Classes
 *******************************************************************************************/
class Gains {
	public:
	
		static Mat overlap_roi[CAMERAS_NUM][2];			// Overlaps ROI (two for each camera: left and right)
		static sem_t th_semaphore;						// Semaphore
		static pthread_mutex_t th_mutex;				// Mutex
		static float gain[CAMERAS_NUM][CHANELS_NUM];	// Gain values
		Compensator* compensator;
	
		/**************************************************************************************************************
		 *
		 * @brief  			Gains class constructor.
		 *
		 * @param	in		int width - display width;
		 *					int height - display height.
		 *
		 * @return 			The function creates the Gains object.
		 *
		 * @remarks 		The function creates Gains object and sets object attributes.
		 *
		 **************************************************************************************************************/
		Gains(int width, int height);
	
		/**************************************************************************************************************
		 *
		 * @brief  			Gains class destructor.
		 *
		 * @param			-
		 *
		 * @return 			-
		 *
		 * @remarks 		The function sets exit flag to the 1 and wait until pdate_gains_th thread will end, then 
		 *					destroys semaphore and releases memory.
		 *
		 **************************************************************************************************************/
		~Gains(void);
	
		/**************************************************************************************************************
		 *
		 * @brief  			Create thread for expsure correction calculation.
		 *
		 * @param			-
		 *
		 * @return 			-
		 *
		 * @remarks 		The function creates thread for expsure correction calculation.
		 *
		 **************************************************************************************************************/
		void updateGains(void);	
	private:
		pthread_t update_gains_th = 0;	// Thread for exposure correction calculation
		static int exit_flag;		// Exit flag
		/**************************************************************************************************************
		 *
		 * @brief  			Calculate exposure correction coefficients.
		 *
		 * @param			-
		 *
		 * @return 			-
		 *
		 * @remarks 		The function calculates exposure correction coefficients in loop. Synchronization is solved 
		 *					by using of mutex and semaphore.
		 *
		 **************************************************************************************************************/
		static void* updateGainsThread(void * input_args);
};

#endif  // GAIN_HPP_

