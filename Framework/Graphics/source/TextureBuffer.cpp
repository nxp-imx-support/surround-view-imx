// Copyright 2025 NXP

#include "TextureBuffer.hpp"

#undef GL_VIV_direct_texture

TextureBuffer::TextureBuffer(Buffer buffer)
    : Texture(buffer.width, buffer.height)
{
    mBuffer = buffer;
    OnUpdate();
}

TextureBuffer::~TextureBuffer()
{
}

void TextureBuffer::Bind()
{
    glBindTexture(mTarget, mId);

    std::lock_guard<std::mutex> guard(mUpdateMutex);
    if (mUpdateRequested) {
        mUpdateRequested = false;
        GLenum format = VideoFormatToGL[mBuffer.format];
        glTexImage2D(mTarget, 0, (GLint)format, (GLsizei)mBuffer.width, (GLsizei)mBuffer.height, 0, format, GL_UNSIGNED_BYTE, mBuffer.start);
    }
}

void TextureBuffer::OnUpdate()
{
    std::lock_guard<std::mutex> guard(mUpdateMutex);
    mUpdateRequested = true;
}

cv::Mat TextureBuffer::GetData()
{
    cv::Mat img(mHeight, mWidth, VideoFormatToCVtype[mBuffer.format], (char*)mBuffer.start);

    // Convert to RGB if needed
    if (mBuffer.format == VideoFormat::RGB) {
        return img;
    } else {
        cv::Mat out;
        cv::cvtColor(img, out, VideoFormatToCVcvt2RGB[mBuffer.format]);
        return out;
    }
}
