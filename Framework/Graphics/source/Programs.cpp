// Copyright 2025 NXP

#include "Programs.hpp"
#include "Log.hpp"
#include "Shaders.hpp"
#include "Texture.hpp"

UTransform::UTransform(Program* program)
    : mProgram(program)
{
}
// Used for MVP or any transformation in vertex shader

void UTransform::SetTransform(const GLfloat* transform)
{
    if (mTransformLoc == -1) {
        mTransformLoc = glGetUniformLocation(mProgram->Handle(), "uTransform");
    }
    glUniformMatrix4fv(mTransformLoc, 1, GL_FALSE, transform);
}

UTexture::UTexture(Program* program)
    : mProgram(program)
{
}

void UTexture::SetTexture(std::shared_ptr<Texture> texture)
{
    // Use sampler 0 for uTexture
    if (mTextureLoc == -1) {
        mTextureLoc = glGetUniformLocation(mProgram->Handle(), "uTexture");
    }
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(mTextureLoc, 0);

    if (texture != nullptr) {
        texture->Bind();
    }
}

UMask::UMask(Program* program)
    : mProgram(program)
{
}

void UMask::SetMask(std::shared_ptr<Texture> texture)
{
    // Use sampler 1 for uMask
    if (mMaskLoc == -1) {
        mMaskLoc = glGetUniformLocation(mProgram->Handle(), "uMask");
    }
    glActiveTexture(GL_TEXTURE1);
    glUniform1i(mMaskLoc, 1);

    if (texture != nullptr) {
        texture->Bind();
    }
}

UGain::UGain(Program* program)
    : mProgram(program)
{
}

void UGain::SetGain(float gain)
{
    SetGain(gain, gain, gain, gain);
}

void UGain::SetGain(float r, float g, float b, float a)
{
    float gains[4] = { r, g, b, a };
    SetGain(gains);
}

void UGain::SetGain(float gain[4])
{
    if (mGainLoc == -1) {
        mGainLoc = glGetUniformLocation(mProgram->Handle(), "uGain");
    }
    glUniform4fv(mGainLoc, 1, gain);
}

ProgramTexturedQuad::ProgramTexturedQuad(const char* uTexture)
    : Program(shaders::s_v_shader, shaders::FragTexture, uTexture)
    , UTexture(this)
    , UGain(this)
{
}

ProgramTexturedQuadExt::ProgramTexturedQuadExt()
    : ProgramTexturedQuad(shaders::TextureExternal)
{
}

ProgramLine::ProgramLine()
    : Program(shaders::s_v_shader_line, shaders::FragColor)
{
}

ProgramGain::ProgramGain(const char* uTexture)
    : Program(shaders::s_v_shader_glm, shaders::FragTexture, uTexture)
    , UTexture(this)
    , UGain(this)
    , UTransform(this)
{
}

ProgramGainExt::ProgramGainExt()
    : ProgramGain(shaders::TextureExternal)
{
}

ProgramGainMask::ProgramGainMask(const char* uTexture)
    : Program(shaders::s_v_shader_glm, shaders::FragTextureMask, uTexture)
    , UTexture(this)
    , UMask(this)
    , UGain(this)
    , UTransform(this)
{
}

ProgramGainMaskExt::ProgramGainMaskExt()
    : ProgramGainMask(shaders::TextureExternal)
{
}

ProgramModel::ProgramModel()
    : Program(shaders::s_v_shader_model, shaders::FragPhong)
    , UTransform(this)
{
}

void ProgramModel::SetAmbient(const GLfloat* color)
{
    if (mAmbientLoc == -1) {
        mAmbientLoc = glGetUniformLocation(mProgram->Handle(), "uAmbient");
    }
    glUniform3fv(mAmbientLoc, 1, color);
}

void ProgramModel::SetDiffuse(const GLfloat* color)
{
    if (mDiffuseLoc == -1) {
        mDiffuseLoc = glGetUniformLocation(mProgram->Handle(), "uDiffuse");
    }
    glUniform3fv(mDiffuseLoc, 1, color);
}

void ProgramModel::SetSpecular(const GLfloat* color)
{
    if (mSpecularLoc == -1) {
        mSpecularLoc = glGetUniformLocation(mProgram->Handle(), "uSpecular");
    }
    glUniform3fv(mSpecularLoc, 1, color);
}

void ProgramModel::SetModelView(const GLfloat* modelview)
{
    if (mModelViewLoc == -1) {
        mModelViewLoc = glGetUniformLocation(mProgram->Handle(), "uModelView");
    }
    glUniformMatrix4fv(mModelViewLoc, 1, GL_FALSE, modelview);
}
void ProgramModel::SetMormalMat(const GLfloat* normalMat)
{
    if (mNormalMatLoc == -1) {
        mNormalMatLoc = glGetUniformLocation(mProgram->Handle(), "uNormalMat");
    }
    glUniformMatrix3fv(mNormalMatLoc, 1, GL_FALSE, normalMat);
}

ProgramBoxQuad::ProgramBoxQuad()
    : Program(shaders::s_v_shader, shaders::FragBox)
    , UGain(this)
{
}
