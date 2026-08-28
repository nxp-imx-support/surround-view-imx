/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Graphics.hpp"
#include "Material.hpp"

#include <memory>
#include <string>
#include <unordered_map>

enum class RenderPrimitive
{
    Point,
    Line,
    Triangle,
};

typedef GLfloat float3[3];
typedef GLfloat float2[2];

class Mesh
{
public:
    Mesh();
    virtual ~Mesh();

    void Load(std::string filename);
    void Load(const GLfloat* data, uint32_t count);
    void LoadVertices(const float3* data, uint32_t count);
    void LoadTexCoord(const float2* data, uint32_t count);
    void LoadNormals(const float3* data, uint32_t count);
    void LoadIndices(const uint32_t* data, uint32_t count);
    void LoadQuad();
    void SetMaterial(std::shared_ptr<Material> material);
    std::shared_ptr<Material> GetMaterial();
    void Render(RenderPrimitive primitive = RenderPrimitive::Triangle);

protected:
    GLuint mVao;
    std::unordered_map<uint32_t, GLuint> mVbos;
    GLuint mEbo = 0;

    uint32_t mCount = 0;
    uint32_t mIndicesCount = 0;

    std::shared_ptr<Material> mMaterial = nullptr;
};
