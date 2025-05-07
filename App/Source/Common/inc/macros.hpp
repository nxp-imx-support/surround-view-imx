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

#ifndef MACROS_HPP_
#define MACROS_HPP_

/*******************************************************************************************
 * Inline
 *******************************************************************************************/
static inline int previous_id(int x, int max_val) {		// Previous index
	int return_value = max_val;
	if(x > 0) {
		return_value = x - 1;
	}
	return return_value;
}

static inline int next_id(int x, int max_val) {		// Next index
	int return_value = 0;
	if(x < max_val) {
		return_value = x + 1;
	}
	return return_value;
}

static inline double timespec2double(timespec t) {		// timespec to doudle
	double return_value = (double)(t.tv_sec) + (1.e-9 * (double)(t.tv_nsec));
	return return_value;
}

static inline double timespec2doublems(timespec t) {		// timespec to doudle
	double return_value = (double)(t.tv_sec) * 1.e3 + (1.e-6 * (double)(t.tv_nsec));
	return return_value;
}

static inline struct timespec timespec_sub(timespec t2, timespec t1) {		// timespec subtraction
	struct timespec sub;
	if (t2.tv_nsec < t1.tv_nsec) {
		sub.tv_nsec = 1000000000 + t2.tv_nsec - t1.tv_nsec;
		sub.tv_sec = t2.tv_sec - t1.tv_sec - 1;
	} 
	else {
		sub.tv_nsec = t2.tv_nsec - t1.tv_nsec;
		sub.tv_sec  = t2.tv_sec  - t1.tv_sec;
	}
	return sub;
}

  
#endif /* MACROS_HPP_ */

