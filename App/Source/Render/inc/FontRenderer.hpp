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
#ifndef FONT_RENDER_HPP_
#define FONT_RENDER_HPP_

#include <fstream>
#include <iomanip>
#include <string>

//Camera movement
#include <glm/glm.hpp> 
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//OpenGL	
#include <GLES3/gl3.h>

//OpenCV
#include "opencv2/highgui/highgui.hpp"

// Spacing between letters
#define FONT_SPACING 25

#define FONT_TEX_GLYPH_COUNT 16
#define FONT_TEX_GLYPH_TOTAL_COUNT FONT_TEX_GLYPH_COUNT * FONT_TEX_GLYPH_COUNT
#define FONT_TEX_SIZE 512
#define FONT_TEX_GLYPH_SIZE FONT_TEX_SIZE/FONT_TEX_GLYPH_COUNT

class FontRenderer
{
protected:
	int screenWidth, screenHeight;
	std::string fontAtlas;
	float size;
		
	GLuint texFont;
	GLuint small_quad, small_quad_tex;
	GLint texLoc, colorLoc, matLoc;
	GLuint shaderProgram;

	
public:
	FontRenderer(int width, int height, std::string atlas);
	~FontRenderer(void);
	
	void Initialize(void);
	
	void RenderText(const char *text, float top, float left);
	void RenderGlyph(int glyphId);
	
	void SetShader(GLuint shader);
	
protected:
	glm::vec3 ComputeFontSize(void);
	float GetRealFontSize(void)
	{
		return ((float)FONT_SPACING * this->size);
	}
};
	
#endif //FONT_RENDER_HPP_

