// Copyright 2025 NXP

#include "TextureBufferVIV.hpp"

TextureBufferVIV::TextureBufferVIV(Buffer buffer)
    : TextureBuffer(buffer)
{
    glBindTexture(mTarget, mId);
    unsigned int physical = ~0U;
    glTexDirectVIVMap(mTarget, mBuffer.width, mBuffer.height, VideoFormatToGL[mBuffer.format],
        (GLvoid**)&mBuffer.start, &physical);
}

TextureBufferVIV::~TextureBufferVIV()
{
}

void TextureBufferVIV::Bind()
{
    glBindTexture(mTarget, mId);

    std::lock_guard<std::mutex> guard(mUpdateMutex);
    if (mUpdateRequested) {
        mUpdateRequested = false;
        glTexDirectInvalidateVIV(mTarget);
    }
}
