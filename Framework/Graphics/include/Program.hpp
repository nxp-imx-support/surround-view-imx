/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>

// OpenGL
#include <GLES3/gl3.h>

namespace shaders {
constexpr const char* GlslVersion = "#version 300 es\n";
constexpr const char* PrecisionMedium = "precision mediump float;\n";
constexpr const char* NoTexture = "\n";
constexpr const char* Texture2D = "uniform sampler2D uTexture; \n";
#ifdef IMX8
constexpr const char* TextureExternal = "uniform sampler2D uTexture; \n";
#else
constexpr const char* TextureExternal = "#extension GL_OES_EGL_image_external : require\n"
                                        "uniform samplerExternalOES uTexture; \n";
#endif
}

class Program
{
public:
    Program(const char* v_shader, const char* p_shader, const char* uTexture = shaders::NoTexture);
    virtual ~Program();
    GLuint Handle(void) { return mHandle; };
    void Use(void);

private:
    GLuint mHandle;
    GLuint mVertexShader;
    GLuint mPixelShader;

    int CompileShader(GLuint id, GLsizei sourceSize, const char** source);
};
