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

#include "ModelLoader/VBO.hpp"

VBO::VBO(Vertex *vertices, Vertex *normals, Coord *texcoords, int count, int materialId)
{
	this->count = (GLuint)count;
	this->matId = materialId;
	this->buffer[0] = 0; this->buffer[1] = 0; this->buffer[2] = 0; this->buffer[3] = 0;
	this->vao = 0;

	//Vertex array
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	//Allocate and assign three VBO to our handle (vertices, normals and texture coordinates)
	glGenBuffers(4, buffer);

	//store vertices into buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffer[P_VERTEX]);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(Vertex) * count, vertices, GL_STATIC_DRAW);
	// vertices are on index 0 and contains three floats per vertex
	glVertexAttribPointer(GLuint(0), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	//store normals into buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffer[P_NORMAL]);
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(Vertex) * count, normals, GL_STATIC_DRAW);
	// normals are on index 1 and contains three floats per vertex
	glVertexAttribPointer(GLuint(1), 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	//store texture coordinates
	glBindBuffer(GL_ARRAY_BUFFER, buffer[P_TEXCOORD]);    
	glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(Coord) * count, texcoords, GL_STATIC_DRAW);

	//coordinates are on index 2 and contains two floats per vertex
	glVertexAttribPointer(GLuint(2), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

VBO::~VBO(void)
{
}
