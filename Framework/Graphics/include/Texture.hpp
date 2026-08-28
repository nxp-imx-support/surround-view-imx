/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Graphics.hpp"

#include <opencv2/opencv.hpp>

class Texture
{
public:
    Texture(int width, int height, GLenum target = GL_TEXTURE_2D);
    Texture(int width, int height, GLuint id, GLenum target);
    virtual ~Texture();

    virtual void SetData(Format format, void* data);
    virtual void SetEmpty(Format format);
    bool IsExternalOES();
    virtual void Bind();
    virtual void OnUpdate();
    GLuint GetId();
    GLenum GetTarget();
    virtual cv::Mat GetData();

protected:
    bool mOwn;
    GLuint mId;
    GLenum mTarget;
    int mWidth;
    int mHeight;
};
