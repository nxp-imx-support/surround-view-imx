// Copyright 2017 NXP

#include "Mesh.hpp"
#include "AssetManager.hpp"
#include "Log.hpp"

#include <algorithm>
#include <fstream>

namespace attribloc {
constexpr GLuint position = 0;
constexpr GLuint texcoord = 1;
constexpr GLuint normals = 2;
} // namespace attribloc

namespace meshes {
// clang-format off
static const GLfloat quad[] = {
    -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
    -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
     1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
     1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
};
// clang-format on
} // namespace meshes

Mesh::Mesh()
{
    glGenVertexArrays(1, &mVao);
}

Mesh::~Mesh(void)
{
    for (auto vbo : mVbos) {
        glDeleteBuffers(1, &vbo.second);
    }
    if (mIndicesCount > 0) {
        glDeleteBuffers(1, &mEbo);
    }
    glDeleteVertexArrays(1, &mVao);
}

void Mesh::Load(std::string filename)
{
    GLfloat* vertices;

    // Read from file
    std::string filePath = AssetManager::GetPath(filename);
    std::ifstream input(filePath.c_str());
    mCount = std::count(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>(), '\n');
    input.clear();
    input.seekg(0, std::ios::beg);

    vertices = (GLfloat*)malloc((size_t)(mCount) * 5U * (size_t)sizeof(GLfloat));
    if (vertices == NULL) {
        LogError("Memory allocation did not complete successfully");
    } else {
        for (int k = 0; k < mCount * 5; k++) {
            input >> vertices[k];
        }
    }
    input.close();

    Load(vertices, mCount);

    if (vertices != NULL) {
        free(vertices);
    }
}

void Mesh::Load(const GLfloat* data, uint32_t count)
{
    // Expect count vertices with position and texcoord (5 floats per vertex)
    if (count == 0 || data == nullptr) {
        LogError("Invalid vertices/texcoord data=%p or count=%d", data, count);
        return;
    }

    mCount = count;
    glBindVertexArray(mVao);

    if (auto vbo = mVbos.find(attribloc::position); vbo == mVbos.end()) {
        GLuint vboHandle;
        glGenBuffers(1, &vboHandle);
        mVbos[attribloc::position] = vboHandle;
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVbos[attribloc::position]);

    // Set GL data
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 5 * mCount, static_cast<const void*>(data),
        GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(attribloc::position);
    glVertexAttribPointer(attribloc::position, 3, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLfloat) * 5, (GLvoid*)0);

    // TexCoord attribute
    glEnableVertexAttribArray(attribloc::texcoord);
    glVertexAttribPointer(attribloc::texcoord, 2, GL_FLOAT, GL_FALSE, (GLsizei)sizeof(GLfloat) * 5,
        (GLvoid*)(3U * sizeof(GLfloat)));

    glBindVertexArray(0);
}

void Mesh::LoadVertices(const float3* data, uint32_t count)
{
    if (count == 0 || data == nullptr) {
        LogError("Invalid vertices data=%p or count=%d", data, count);
        return;
    }

    mCount = count;
    glBindVertexArray(mVao);

    if (auto vbo = mVbos.find(attribloc::position); vbo == mVbos.end()) {
        GLuint vboHandle;
        glGenBuffers(1, &vboHandle);
        mVbos[attribloc::position] = vboHandle;
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVbos[attribloc::position]);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 3 * mCount, data, GL_STATIC_DRAW);

    glVertexAttribPointer(attribloc::position, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attribloc::position);

    glBindVertexArray(0);
}

void Mesh::LoadTexCoord(const float2* data, uint32_t count)
{
    if (count == 0 || data == nullptr) {
        LogError("Invalid texture coordinates data=%p or count=%d", data, count);
        return;
    }

    mCount = count;
    glBindVertexArray(mVao);

    if (auto vbo = mVbos.find(attribloc::texcoord); vbo == mVbos.end()) {
        GLuint vboHandle;
        glGenBuffers(1, &vboHandle);
        mVbos[attribloc::texcoord] = vboHandle;
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVbos[attribloc::texcoord]);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 2 * mCount, data, GL_STATIC_DRAW);

    glVertexAttribPointer(attribloc::texcoord, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attribloc::texcoord);

    glBindVertexArray(0);
}

void Mesh::LoadNormals(const float3* data, uint32_t count)
{
    if (count == 0 || data == nullptr) {
        LogError("Invalid normals data=%p or count=%d", data, count);
        return;
    }

    mCount = count;
    glBindVertexArray(mVao);

    if (auto vbo = mVbos.find(attribloc::normals); vbo == mVbos.end()) {
        GLuint vboHandle;
        glGenBuffers(1, &vboHandle);
        mVbos[attribloc::normals] = vboHandle;
    }
    glBindBuffer(GL_ARRAY_BUFFER, mVbos[attribloc::normals]);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)sizeof(GLfloat) * 3 * mCount, data, GL_STATIC_DRAW);

    glVertexAttribPointer(attribloc::normals, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(attribloc::normals);

    glBindVertexArray(0);
}

void Mesh::LoadIndices(const uint32_t* data, uint32_t count)
{
    if (count == 0 || data == nullptr) {
        LogError("Invalid indices data=%p or count=%d", data, count);
        return;
    }
    glBindVertexArray(mVao);

    if (mIndicesCount == 0) {
        glGenBuffers(1, &mEbo);
    }
    mIndicesCount = count;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIndicesCount * sizeof(uint32_t), data, GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void Mesh::LoadQuad()
{
    Load(meshes::quad, 6);
}

void Mesh::SetMaterial(std::shared_ptr<Material> material)
{
    mMaterial = material;
}

std::shared_ptr<Material> Mesh::GetMaterial()
{
    return mMaterial;
}

void Mesh::Render(RenderPrimitive primitive)
{
    glBindVertexArray(mVao);
    switch (primitive) {
    case RenderPrimitive::Line:
        glLineWidth(2.0f);
        if (mIndicesCount > 0) {
            glDrawElements(GL_LINES, mIndicesCount, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_LINES, 0, mCount);
        }
        break;
    case RenderPrimitive::Point:
        glBeginTransformFeedback(GL_POINTS);
        if (mIndicesCount > 0) {
            glDrawElements(GL_POINTS, mIndicesCount, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_POINTS, 0, mCount);
        }
        glEndTransformFeedback();
        break;
    case RenderPrimitive::Triangle:
        if (mIndicesCount > 0) {
            glDrawElements(GL_TRIANGLES, mIndicesCount, GL_UNSIGNED_INT, 0);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, mCount);
        }
        break;
    default:
        LogWarning("Unknown render primitive");
        break;
    }
    glBindVertexArray(0);
}
