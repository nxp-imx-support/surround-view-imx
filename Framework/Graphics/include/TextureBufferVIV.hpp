/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "TextureBuffer.hpp"

class TextureBufferVIV : public TextureBuffer
{
public:
    TextureBufferVIV(Buffer buffer);
    virtual ~TextureBufferVIV();

    // Inherited from TextureBuffer
    virtual void Bind() override;
};
