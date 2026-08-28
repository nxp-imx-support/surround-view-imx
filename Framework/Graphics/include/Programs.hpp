/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Program.hpp"
#include <memory>

class Texture;

class UTransform
{
public:
    UTransform(Program* program);
    // Used for MVP or any transformation in vertex shader
    void SetTransform(const GLfloat* transform);

protected:
    Program* mProgram = nullptr;
    GLint mTransformLoc = -1;
};

class UTexture
{
public:
    UTexture(Program* program);
    void SetTexture(std::shared_ptr<Texture> texture);

protected:
    Program* mProgram = nullptr;
    GLint mTextureLoc = -1;
};

class UMask
{
public:
    UMask(Program* program);
    void SetMask(std::shared_ptr<Texture> texture);

protected:
    Program* mProgram = nullptr;
    GLint mMaskLoc = -1;
};

class UGain
{
public:
    UGain(Program* program);
    void SetGain(float gain);
    void SetGain(float r, float g, float b, float a);
    void SetGain(float gain[4]);

protected:
    Program* mProgram = nullptr;
    GLint mGainLoc = -1;
};

class ProgramTexturedQuad : public Program, public UTexture, public UGain
{
public:
    ProgramTexturedQuad(const char* uTexture = shaders::Texture2D);
};

class ProgramTexturedQuadExt : public ProgramTexturedQuad
{
public:
    ProgramTexturedQuadExt();
};

class ProgramLine : public Program
{
public:
    ProgramLine();
};

class ProgramGain : public Program, public UTexture, public UGain, public UTransform
{
public:
    ProgramGain(const char* uTexture = shaders::Texture2D);
};

class ProgramGainExt : public ProgramGain
{
public:
    ProgramGainExt();
};

class ProgramGainMask : public Program, public UTexture, public UMask, public UGain, public UTransform
{
public:
    ProgramGainMask(const char* uTexture = shaders::Texture2D);
};

class ProgramGainMaskExt : public ProgramGainMask
{
public:
    ProgramGainMaskExt();
};

class ProgramModel : public Program, public UTransform
{
public:
    ProgramModel();

    void SetAmbient(const GLfloat* color);
    void SetDiffuse(const GLfloat* color);
    void SetSpecular(const GLfloat* color);
    void SetModelView(const GLfloat* modelview);
    void SetMormalMat(const GLfloat* normalMat);

protected:
    GLint mAmbientLoc = -1;
    GLint mDiffuseLoc = -1;
    GLint mSpecularLoc = -1;
    GLint mModelViewLoc = -1;
    GLint mNormalMatLoc = -1;
};

class ProgramBoxQuad : public Program, public UGain
{
public:
    ProgramBoxQuad();
};
