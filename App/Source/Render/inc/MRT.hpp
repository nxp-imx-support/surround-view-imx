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

#ifndef MRT_HPP_
#define MRT_HPP_

#include <GLES3/gl3.h>

#define MRT_ENABLED 1

class MRT
{
protected:
	int width, height;

	GLuint small_quad;
	GLuint small_quad_tex;

	GLuint fbo, depth;
	GLuint screenTex, videoTex;

	bool enabled;

public:
	MRT(int mrtWidth, int mrtHeight);
	~MRT(void);

	GLuint getFBO(void){ return fbo; }

	void Initialize(void);

	void RenderSmallQuad(GLuint showTexP);
	bool isEnabled(void) { return ((MRT_ENABLED != 0) && enabled); }
};

#endif

