/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Programs.hpp"

#include <fstream>
#include <iomanip>
#include <memory>
#include <string>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// OpenGL
#include <GLES3/gl3.h>

// OpenCV
#include "opencv2/highgui/highgui.hpp"

// Spacing between letters
#define FONT_SPACING 25

#define FONT_TEX_GLYPH_COUNT 16
#define FONT_TEX_GLYPH_TOTAL_COUNT FONT_TEX_GLYPH_COUNT* FONT_TEX_GLYPH_COUNT
#define FONT_TEX_SIZE 512
#define FONT_TEX_GLYPH_SIZE FONT_TEX_SIZE / FONT_TEX_GLYPH_COUNT

class Texture;

class FontRenderer
{
protected:
    int screenWidth, screenHeight;
    std::string fontAtlas;
    float size;

    ProgramGain mProgram;
    std::shared_ptr<Texture> texFont;
    GLuint small_quad, small_quad_tex;

public:
    FontRenderer(int width, int height, std::string atlas);
    ~FontRenderer(void);

    void RenderText(const char* text, float top, float left);
    void RenderGlyph(int glyphId);

protected:
    glm::vec3 ComputeFontSize(void);
    float GetRealFontSize(void) { return ((float)FONT_SPACING * this->size); }
};
