// Copyright 2017 NXP

#include "FontRenderer.hpp"
#include "AssetManager.hpp"
#include "Log.hpp"
#include "Texture.hpp"

#include <opencv2/imgcodecs/legacy/constants_c.h>

FontRenderer::FontRenderer(int width, int height, std::string atlas)
    : screenWidth(width)
    , screenHeight(height)
    , fontAtlas(atlas)
    , size(1.0f)
    , small_quad(0)
    , small_quad_tex(0)
{
    LogInfo("Loading Font atlas: %s", fontAtlas.c_str());
    cv::Mat img = cv::imread(AssetManager::GetPath(fontAtlas), cv::IMREAD_GRAYSCALE);

    texFont = std::make_shared<Texture>(img.cols, img.rows);
    texFont->SetData(Format::GREY8, img.data);

    // Init data for texture quads
    GLfloat vertattribs[] = { 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, -1.0f, 1.0f, 0.0f };
    GLfloat texattribs[] = { 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    glGenBuffers(1, &small_quad);
    glBindBuffer(GL_ARRAY_BUFFER, small_quad);
    glBufferData(GL_ARRAY_BUFFER, 12L * (GLsizeiptr)sizeof(GLfloat), &vertattribs, GL_STATIC_DRAW);
    glVertexAttribPointer(GLuint(0), 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    // also texcoords
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
    glDeleteBuffers(1, &small_quad);
    glDeleteBuffers(1, &small_quad_tex);
}

void FontRenderer::RenderText(const char* text, float top, float left)
{
    if (top < 0.0f || left < 0.0f) {
        return;
    }

    // Compute text position on screen in normalized coordinates (-1..1)
    float tx = GetRealFontSize() / (float)screenWidth + 2.0f * (left / 100.0f);
    float ty = GetRealFontSize() / (float)screenHeight + 2.0f * (top / 100.0f);

    glDisable(GL_DEPTH_TEST);

    mProgram.Use();
    mProgram.SetTexture(texFont);

    glm::vec4 color = glm::vec4(0.0, 1.0, 0.0, 1.0);
    mProgram.SetGain(glm::value_ptr(color));

    glm::vec3 font_scale = ComputeFontSize();
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0 + tx, 1.0 - ty, 0.0));

    const char* p;
    for (p = text; *p != '\0'; p++) {
        int glyphId = int(*p);

        glm::mat4 transMat = glm::scale(translation, font_scale);
        mProgram.SetTransform(glm::value_ptr(transMat));

        RenderGlyph(glyphId);
        translation = glm::translate(translation, glm::vec3(2.0f * GetRealFontSize() / (float)screenWidth, 0.0, 0.0));
    }

    glEnable(GL_DEPTH_TEST);
}

void FontRenderer::RenderGlyph(int glyphId)
{
    int i = glyphId / FONT_TEX_GLYPH_COUNT; // i-th row in the font texture
    int j = glyphId % FONT_TEX_GLYPH_COUNT; // j-th column in the font texture

    float normSize = ((float)FONT_TEX_GLYPH_SIZE) / ((float)FONT_TEX_SIZE);

    // update texture attributes
    float texLeft = (float)j * normSize;
    float texRight = texLeft + normSize;
    float texTop = (float)i * normSize;
    float texBottom = texTop + normSize;

    GLfloat texattribs[] = { texRight, texBottom, texRight, texTop, texLeft, texBottom, texLeft, texTop };

    glBindBuffer(GL_ARRAY_BUFFER, small_quad);
    glVertexAttribPointer(GLuint(0), 3, GL_FLOAT, GL_FALSE, 0, 0); // bind attributes to index

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
