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

#include "MRT.hpp"

#include <iostream>
#include <assert.h>

#include "shaders.hpp"

using namespace std;

MRT::MRT(int mrtWidth, int mrtHeight)
	: width(mrtWidth), height(mrtHeight), enabled(false)
{
	this->small_quad = 0;
	this->small_quad_tex = 0;
	this->fbo = 0;
	this->depth = 0;
	this->screenTex = 0;
	this->videoTex = 0;
}

void MRT::Initialize(void)
{
	enabled = true;

	//generate FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenRenderbuffers(1, &depth);
	glBindRenderbuffer( GL_RENDERBUFFER, depth );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height );

	//color texture
	glGenTextures(1, &screenTex);
	glBindTexture(GL_TEXTURE_2D, screenTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	//normal texture
	glGenTextures(1, &videoTex);
	glBindTexture(GL_TEXTURE_2D, videoTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,  width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	//bind texture to FBO
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, videoTex, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER, depth);

	// Test FrameBuffer completness
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != (GLenum)GL_FRAMEBUFFER_COMPLETE)
	{
		cerr << "FrameBuffer initialization failed. Error: " << status << endl;
		enabled = false;
	}

	// Unbind FBO
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	// Init data for texture quads
	const GLfloat vertattribs[] = { 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f };
	const GLfloat texattribs[] = { 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	glGenBuffers(1, &small_quad);
	glBindBuffer(GL_ARRAY_BUFFER, small_quad);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 8, &vertattribs, GL_STATIC_DRAW);
	glVertexAttribPointer(GLuint(0), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	//also texcoords
	glGenBuffers(1, &small_quad_tex);
	glBindBuffer(GL_ARRAY_BUFFER, small_quad_tex);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 8, &texattribs, GL_STATIC_DRAW);
	glVertexAttribPointer(GLuint(1), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	assert(GL_NO_ERROR == glGetError());

	cout << "MRT Support: " << (this->isEnabled() ? "Enabled" : "Disabled") << endl;
}

MRT::~MRT(void)
{
	//Delete resources
	glDeleteTextures(1, &screenTex);
	glDeleteTextures(1, &videoTex);
	glDeleteRenderbuffers(1, &depth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &fbo);
}

void MRT::RenderSmallQuad(GLuint showTexP)
{
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, screenTex);
	
	glUseProgram( showTexP );
	
	GLint texLoc = glGetUniformLocation(showTexP, "tex");
	glUniform1i(texLoc, 0);

	glBindBuffer(GL_ARRAY_BUFFER, small_quad);
	glVertexAttribPointer(GLuint(0), 2, GL_FLOAT, GL_FALSE, 0, 0);  //bind attributes to index

	glBindBuffer(GL_ARRAY_BUFFER, small_quad_tex);
	glVertexAttribPointer(GLuint(1), 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glUseProgram( 0 );
	
	glBindTexture(GL_TEXTURE_2D, 0);
}
