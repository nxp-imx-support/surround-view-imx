// Copyright 2025 NXP

#include "Texture.hpp"

Texture::Texture(int width, int height, GLenum target)
    : mOwn(true)
    , mWidth(width)
    , mHeight(height)
    , mTarget(target)
{
    glGenTextures(1, &mId);

    glBindTexture(mTarget, mId);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::Texture(int width, int height, GLuint id, GLenum target)
    : mId(id)
    , mTarget(target)
    , mOwn(false)
    , mWidth(width)
    , mHeight(height)
{
    glBindTexture(mTarget, mId);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(mTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(mTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(mTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

Texture::~Texture()
{
    if (mOwn) {
        glDeleteTextures(1, &mId);
    }
}

void Texture::SetData(Format format, void* data)
{
    glBindTexture(mTarget, mId);
    glTexImage2D(mTarget, 0, Graphics::GetGLInternalFormat(format), mWidth, mHeight, 0, Graphics::GetGLFormat(format), GL_UNSIGNED_BYTE, data);
    glBindTexture(mTarget, 0);
}

void Texture::SetEmpty(Format format)
{
    glBindTexture(mTarget, mId);
    glTexStorage2D(mTarget, 1, Graphics::GetGLInternalFormat(format), mWidth, mHeight);
    glBindTexture(mTarget, 0);
}

bool Texture::IsExternalOES()
{
    return (mTarget == GL_TEXTURE_EXTERNAL_OES);
}

void Texture::Bind()
{
    glBindTexture(mTarget, mId);
}

void Texture::OnUpdate()
{
}

GLuint Texture::GetId()
{
    return mId;
}

GLenum Texture::GetTarget()
{
    return mTarget;
}

cv::Mat Texture::GetData()
{
    GLubyte* data = new GLubyte[mWidth * mHeight * 4];

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, mTarget, mId, 0);
    glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindTexture(mTarget, 0);

    cv::Mat rgba = cv::Mat(mHeight, mWidth, CV_8UC4, data);
    cv::Mat out = cv::Mat(mHeight, mWidth, CV_8UC3);
    cvtColor(rgba, out, cv::COLOR_RGBA2BGR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);

    delete[] data;
    return out;
}
