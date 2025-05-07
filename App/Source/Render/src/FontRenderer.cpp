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

#include "FontRenderer.hpp"
#include <opencv2/imgcodecs/legacy/constants_c.h>

using namespace std;
using namespace cv;

FontRenderer::FontRenderer(int width, int height, string atlas)
{
	this->screenWidth = width;
	this->screenHeight = height;
	this->fontAtlas= atlas;
	this->size = 1.0f;
	this->texFont = 0;
	this->small_quad = 0;
	this->small_quad_tex = 0;
	this->shaderProgram = 0;
	this->texLoc = 0;
	this->colorLoc = 0;
	this->matLoc = 0;
	

}

void FontRenderer::Initialize(void)
{
	glGenTextures(1, &texFont);
		
	glBindTexture(GL_TEXTURE_2D, texFont);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	Mat img = imread(fontAtlas.c_str(), CV_LOAD_IMAGE_GRAYSCALE);
	
	assert(img.data != NULL);
		
	glBindTexture(GL_TEXTURE_2D, texFont);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, img.cols, img.rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img.data);
	glBindTexture(GL_TEXTURE_2D, 0);
	
	assert(GL_NO_ERROR == glGetError() && "An error occured during font texture loading.");

	// Init data for texture quads
	GLfloat vertattribs[] = { 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f };
	GLfloat texattribs[] = { 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	
	glGenBuffers(1, &small_quad);
	glBindBuffer(GL_ARRAY_BUFFER, small_quad);
	glBufferData(GL_ARRAY_BUFFER, 8L * (GLsizeiptr)sizeof(GLfloat), &vertattribs, GL_STATIC_DRAW);
	glVertexAttribPointer(GLuint(0), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
	//also texcoords
	glGenBuffers(1, &small_quad_tex);
	glBindBuffer(GL_ARRAY_BUFFER, small_quad_tex);
	glBufferData(GL_ARRAY_BUFFER, 8L * (GLsizeiptr)sizeof(GLfloat), &texattribs, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(GLuint(1), 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	assert(GL_NO_ERROR == glGetError() && "An error occured during font VBO loading.");
}

FontRenderer::~FontRenderer(void)
{
	glDeleteTextures(1, &texFont);	
	glDeleteBuffers(1, &small_quad);
	glDeleteBuffers(1, &small_quad_tex);
}

void FontRenderer::SetShader(GLuint shader)
{
	assert(shader != 0U);

	shaderProgram = shader;

	texLoc = glGetUniformLocation(shaderProgram, "tex");
	assert(texLoc != -1);
	colorLoc = glGetUniformLocation(shaderProgram, "color");
	assert(colorLoc != -1);
	matLoc = glGetUniformLocation(shaderProgram, "mat");
	assert(matLoc != -1);
}

void FontRenderer::RenderText(const char *text, float top, float left)
{
	if (top < 0.0f || left < 0.0f) {
		return; }
	
	if (shaderProgram == 0U) {
		return; }
	
	//calculate real text position on a screen
	float tx = GetRealFontSize() / (float)screenWidth + 2.0f * (left / 100.0f); // 2.0 is the size of a screen in normalized coordinates (-1..1)
	//TODO: Compute correct value. Where is 0? Is the border included?
	float ty = 0.1f; 
	
	glDisable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, texFont);
	
	glUseProgram(shaderProgram);
	glUniform1i(texLoc, 0);
	glm::vec4 color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	glUniform4fv(colorLoc, 1, glm::value_ptr(color));
	
	glm::vec3 font_scale = ComputeFontSize();
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0 + tx, 1.0 - ty, 0.0));
	
	const char *p;
	for (p = text; *p != '\0'; p++) 
	{
		int glyphId = int(*p);
		
		glm::mat4 transMat = glm::scale(translation, font_scale);
		glUniformMatrix4fv(matLoc, 1, GL_FALSE, glm::value_ptr(transMat)); 
		
		RenderGlyph(glyphId);	
		translation = glm::translate(translation, glm::vec3(2.0f * GetRealFontSize() / (float)screenWidth, 0.0, 0.0));
	}
	
	glUseProgram(0);
	

	glBindTexture(GL_TEXTURE_2D, 0);
	
	glEnable(GL_DEPTH_TEST);
}

void FontRenderer::RenderGlyph(int glyphId)
{
	int i = glyphId / FONT_TEX_GLYPH_COUNT; // i-th row in the font texture
	int j = glyphId % FONT_TEX_GLYPH_COUNT; // j-th column in the font texture
	
	float normSize = ((float)FONT_TEX_GLYPH_SIZE) / ((float)FONT_TEX_SIZE);
    
	//update texture attributes
	float texLeft = (float)j * normSize;
	float texRight = texLeft + normSize;
	float texTop = (float)i * normSize;
	float texBottom = texTop + normSize;
	
	GLfloat texattribs[] = { texRight, texBottom, texRight, texTop, texLeft, texBottom, texLeft, texTop };

	glBindBuffer(GL_ARRAY_BUFFER, small_quad);
	glVertexAttribPointer(GLuint(0), 2, GL_FLOAT, GL_FALSE, 0, 0);  //bind attributes to index

	glBindBuffer(GL_ARRAY_BUFFER, small_quad_tex);
	glBufferData(GL_ARRAY_BUFFER, 8L * (GLsizeiptr)sizeof(GLfloat), &texattribs, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(GLuint(1), 2, GL_FLOAT, GL_FALSE, 0, 0);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

glm::vec3 FontRenderer::ComputeFontSize(void)
{
	float sx = GetRealFontSize() / (float)screenWidth;
	float sy = GetRealFontSize() / (float)screenHeight;
	
	return glm::vec3(sx, sy, 1.0);	
}
