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

#include "gain.hpp"


float Gains::gain[CAMERAS_NUM][CHANELS_NUM] = { 0.0f };
pthread_mutex_t Gains::th_mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t Gains::th_semaphore;
int Gains::exit_flag = 0;
Mat Gains::overlap_roi[CAMERAS_NUM][2];


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
Gains::Gains(int width, int height)
{
	sem_init(&Gains::th_semaphore, 0, 0);
	Gains::exit_flag = 0;
	for (int i = 0; i < CAMERAS_NUM; i++) {
		for (int j = 0; j < CHANELS_NUM; j++) {
			Gains::gain[i][j] = 1.0f; 
		}
	}
	

	// Load compensator
	compensator = new Compensator(Size(width, height));
	compensator->load((const char*)"./compensator");
	
	for (int j = 0; j < CAMERAS_NUM; j++)
	{	
		Gains::overlap_roi[j][0] = Mat(compensator->getFlipROI((uint)j).height, compensator->getFlipROI((uint)j).width, CV_8UC(4));
		uint next = (uint)next_id(j, CAMERAS_NUM - 1);
		Gains::overlap_roi[j][1] = Mat(compensator->getFlipROI(next).height, compensator->getFlipROI(next).width, CV_8UC(4));
	}
}

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
Gains::~Gains(void)
{
	Gains::exit_flag = 1;
	void* status = 0;
	
	sem_post(&Gains::th_semaphore); // Release gain semaphore
	
	pthread_join(update_gains_th, &status);
	if (status != 0) {
		cout << "Exposure correction thread join failed" << endl; 
	}
	
	// Destroy semaphore
	if(sem_destroy(&Gains::th_semaphore) == -1) {
		cout << "sem_destroy error" << endl; 
	}
	delete(compensator);
}

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
void Gains::updateGains(void)
{
	if(pthread_create(&update_gains_th, NULL, Gains::updateGainsThread, (void *)0) !=0) {
		cout << "Cannot create exposure correction thread" << endl;
	}
}


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
void* Gains::updateGainsThread(void * input_args)
{
	struct timespec t1 = {0, 0};
	static struct timespec t2 = {0, 0};

	double gamma = 2.2;
	double gamma_inv = 1.0 / gamma;	
		
	while (exit_flag == 0)
	{
		// Whait on Exposure Correction call
		sem_wait(&th_semaphore);
		
		if(exit_flag != 0) { return 0; }

		// Lock gain
		pthread_mutex_lock(&th_mutex);
		
		clock_gettime(CLOCK_REALTIME, &t1);
			
		Scalar Acc_left[CAMERAS_NUM], Acc_right[CAMERAS_NUM];
		
		for (uint camera = 0U; camera < (uint)CAMERAS_NUM; ++camera) 
		{
			Scalar Ci_left(0, 0, 0), Ci_right(0, 0, 0);

			for (int col = 0; col < overlap_roi[camera][0].cols; col++) 
			{
				for (int row = 0; row < overlap_roi[camera][0].rows; row++) 
				{
					// Ci_left += images(col, row) ^ gamma * mask(col, row)
					Scalar img_pow = (Scalar)overlap_roi[camera][0].at<Vec4b>(Point(col, row));
					pow(img_pow, gamma, img_pow);
					add(img_pow, Ci_left, Ci_left);
				}
			}
			Acc_left[camera] = Ci_left;


			for (int col = 0; col < overlap_roi[camera][1].cols; col++) 
			{
				for (int row = 0; row < overlap_roi[camera][1].rows; row++) 
				{
					// Ci_right += images(col, row) ^ gamma * mask(col, row)
					Scalar img_pow = (Scalar)overlap_roi[camera][1].at<Vec4b>(Point(col, row));
					pow(img_pow, gamma, img_pow);
					add(img_pow, Ci_right, Ci_right);
				}
			}
			Acc_right[camera] = Ci_right;
		}


		Scalar a[CAMERAS_NUM]; // Color correction coefficient
		a[0] = Scalar(1, 1, 1); // For the first image the color correction coefficient is 1 for all channels
		for (uint i = 1U; i < (uint)CAMERAS_NUM; ++i) 
		{
			divide(Acc_right[i - 1U], Acc_left[i], a[i]);
		}
	

		double ar = 0.0, ag = 0.0, ab = 0.0, ar_2 = 0.0, ag_2 = 0.0, ab_2 = 0.0;
		for (uint i = 0U; i < (uint)CAMERAS_NUM; ++i) 
		{
			ar += a[i].val[0]; // Color correction coefficients for channel R
			ag += a[i].val[1]; // Color correction coefficients for channel G
			ab += a[i].val[2]; // Color correction coefficients for channel B
			ar_2 += a[i].val[0] * a[i].val[0]; // ar^2
			ag_2 += a[i].val[1] * a[i].val[1]; // ag^2
			ab_2 += a[i].val[2] * a[i].val[2]; // ab^2
		}


		Scalar g(ar / ar_2, ag / ag_2, ab / ab_2); // Global compensation coefficient
		for (uint i = 0U; i < (uint)CAMERAS_NUM; ++i)
		{			
			multiply(g, a[i], a[i]);
			pow(a[i], gamma_inv, a[i]);
		}
				
		for (uint camera = 0U; camera < (uint)CAMERAS_NUM; ++camera)
		{
			for (int color = 0; color < 3; color++)
			{
				gain[camera][color] = (float)a[camera][2 - color];
			}
		}
			
		clock_gettime(CLOCK_REALTIME, &t2);
		struct timespec diff = timespec_sub(t2, t1);
		double t = timespec2double(diff);
		cout << "Time rate: " << t * 1000.0 << " ms" << endl;	
			
		if(pthread_mutex_unlock(&th_mutex) !=0) {
			cout << "pthread_mutex_unlock error" << endl;	
		}
	}
	return 0;
}

