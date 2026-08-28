/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif

// clang-format off
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
// clang-format on

enum class Format
{
    GREY8,
    RGB8,
    RGBA8,
};

class Graphics
{
public:
    static inline GLuint GetGLFormat(Format format)
    {
        GLenum glFormat;
        switch (format) {
        case Format::GREY8:
            glFormat = GL_LUMINANCE;
            break;
        case Format::RGB8:
            glFormat = GL_RGB;
            break;
        case Format::RGBA8:
            glFormat = GL_RGBA;
            break;
        default:
            glFormat = GL_RGB;
            break;
        }
        return glFormat;
    }

    static inline GLenum GetGLInternalFormat(Format format)
    {
        // Assume always using uint_8 data type
        GLenum glInternalFormat;
        switch (format) {
        case Format::GREY8:
            glInternalFormat = GL_LUMINANCE;
            break;
        case Format::RGB8:
            glInternalFormat = GL_RGB8;
            break;
        case Format::RGBA8:
            glInternalFormat = GL_RGBA8;
            break;
        default:
            glInternalFormat = GL_RGB8;
            break;
        }
        return glInternalFormat;
    }
};

#if USE_VIV

// GL_VIV_direct_texture
#ifndef GL_VIV_direct_texture
#define GL_VIV_direct_texture 1
#define GL_VIV_YV12 0x8FC0
#define GL_VIV_NV12 0x8FC1
#define GL_VIV_YUY2 0x8FC2
#define GL_VIV_UYVY 0x8FC3
#define GL_VIV_NV21 0x8FC4
#define GL_VIV_I420 0x8FC5
#define GL_VIV_AYUV 0x8FC6
#define GL_VIV_YUV420_10_ST 0x8FC7
#define GL_VIV_YUV420_TILE_ST 0x8FC8
#define GL_VIV_YUV420_TILE_10_ST 0x8FC9

#ifdef GL_GLEXT_PROTOTYPES
GL_APICALL void GL_APIENTRY glTexDirectVIVMap(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Logical, const GLuint* Physical);
GL_APICALL void GL_APIENTRY glTexDirectMapVIV(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Logical, const GLuint* Physical);
GL_APICALL void GL_APIENTRY glTexDirectVIV(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Pixels);
GL_APICALL void GL_APIENTRY glTexDirectInvalidateVIV(GLenum Target);
GL_APICALL void GL_APIENTRY glTexDirectTiledMapVIV(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Logical, const GLuint* Physical);
#endif
typedef void(GL_APIENTRYP PFNGLTEXDIRECTVIVMAPPROC)(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Logical, const GLuint* Physical);
typedef void(GL_APIENTRYP PFNGLTEXDIRECTMAPVIVPROC)(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Logical, const GLuint* Physical);
typedef void(GL_APIENTRYP PFNGLTEXDIRECTVIVPROC)(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Pixels);
typedef void(GL_APIENTRYP PFNGLTEXDIRECTINVALIDATEVIVPROC)(GLenum Target);
typedef void(GL_APIENTRYP PFNGLTEXDIRECTTILEDMAPVIVPROC)(GLenum Target, GLsizei Width, GLsizei Height, GLenum Format,
    GLvoid** Logical, const GLuint* Physical);

#ifdef ANDROID
extern PFNGLTEXDIRECTVIVMAPPROC pFNglTexDirectVIVMap;
extern PFNGLTEXDIRECTINVALIDATEVIVPROC pFNglTexDirectInvalidateVIV;
#define glTexDirectVIVMap (*pFNglTexDirectVIVMap)
#define glTexDirectInvalidateVIV (*pFNglTexDirectInvalidateVIV)
#endif

#endif // GL_VIV_direct_texture
#endif
