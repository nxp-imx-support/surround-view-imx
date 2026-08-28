// Copyright 2017, 2026 NXP

#include "Model.hpp"
#include "AssetManager.hpp"
#include "FilesName.hpp"
#include "Log.hpp"
#include "Programs.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

struct MeshBuffers
{
    uint32_t indicesCount = 0;
    uint32_t* indices = nullptr;
    uint32_t verticesCount = 0;
    float3* vertices = nullptr;
    float2* texcoords = nullptr;
    float3* normals = nullptr;
    uint32_t material = 0;
};

Model::Model(void)
{
    if (pthread_create(&mThread, NULL, Model::Load, (void*)this) != 0) {
        LogError("Cannot create load model thread");
    }
}

Model::~Model(void)
{
    // Wait for initialization to be done before deinit
    if (mThread != 0) {
        void* status = 0;
        pthread_join(mThread, &status);
        mThread = 0;
        if (status != 0) {
            LogError("Model load initialization failed");
        }
    }

    mIsInitialized = false;
}

std::string Model::GetModelFileName(void)
{
    std::string line;
    std::string filePath = AssetManager::GetPath(std::string(MODEL_PATH_FILE));
    std::ifstream modelFile(filePath.c_str());
    if (modelFile.is_open()) {
        std::stringstream buffer;
        buffer << modelFile.rdbuf();
        buffer >> line;
    } else {
        LogError("Unable to open file: %s", MODEL_PATH_FILE);
    }

    return line;
}

void* Model::Load(void* userData)
{
    Model* instance = (Model*)userData;

    std::string filename = instance->GetModelFileName();
    if (filename.empty()) {
        LogError("Model file name is empty");
        return (void*)(-1);
    }

    std::string filepath = MODELS_DIR + filename;

    LogInfo("Loading model: %s", filepath.c_str());
    Assimp::Importer importer;

    // Do not import line and point meshes
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);

    const aiScene* aiscene = importer.ReadFile(
        AssetManager::GetPath(filepath),
        aiProcess_JoinIdenticalVertices | aiProcess_LimitBoneWeights | aiProcess_PreTransformVertices | aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_SortByPType | aiProcess_FindDegenerates | aiProcess_FindInvalidData | aiProcess_GenNormals);

    if (aiscene == nullptr) {
        LogError("Cannot load scene from file %s", filepath.c_str());
        return (void*)(-1);
    }
    // Load materials
    for (unsigned int i = 0; i < aiscene->mNumMaterials; i++) {
        // materials
        aiMaterial* m = aiscene->mMaterials[i];

        // get material name
        aiString name;
        m->Get(AI_MATKEY_NAME, name);

        // get material properties
        aiColor3D ambient, diffuse, specular;
        float shininess = 0.0f;

        aiReturn r;
        r = m->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
        if (r != AI_SUCCESS) {
            LogError("Cannot load color ambient property");
        }
        r = m->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
        if (r != AI_SUCCESS) {
            LogError("Cannot load color diffuse property");
        }
        r = m->Get(AI_MATKEY_COLOR_SPECULAR, specular);
        if (r != AI_SUCCESS) {
            LogError("Cannot load color specular property");
        }
        r = m->Get(AI_MATKEY_SHININESS, shininess);
        if (r != AI_SUCCESS) {
            LogError("Cannot load shininess property");
        }

        auto material = std::make_shared<Material>(glm::vec3(ambient.r, ambient.g, ambient.b), glm::vec3(diffuse.r, diffuse.g, diffuse.b),
            glm::vec3(specular.r, specular.g, specular.b), glm::max(shininess, 255.0f));
        instance->mMaterials.push_back(material);

        // Loads up base textures for material
        // TODO: What to do when the count is greater than 1
        unsigned int numTex = m->GetTextureCount(aiTextureType_DIFFUSE);
        for (unsigned int j = 0; j < numTex; ++j) {
            // Textures
            std::string path;
            aiString pth;
            aiReturn texFound;

            // Get ambient textures
            texFound = m->GetTexture(aiTextureType_DIFFUSE, j, &pth);
            if (texFound == AI_FAILURE) {
                break;
            }

            path += pth.C_Str();

            // TODO: implement to method
            LogInfo("Texture: %s", path.c_str());
        }
    }

    LogInfo("Loading scene (%u meshes): ", aiscene->mNumMeshes);

    // Load meshes
    aiMesh* aimesh;
    unsigned int polygons = 0;
    for (unsigned int m = 0; m < aiscene->mNumMeshes; ++m) {
        aimesh = aiscene->mMeshes[m];
        polygons += aimesh->mNumFaces;

        if (aimesh->mNumFaces == 0U) {
            continue;
        }

        std::shared_ptr<MeshBuffers> meshBuffers = std::make_shared<MeshBuffers>();

        // Indices
        meshBuffers->indicesCount = aimesh->mNumFaces * 3;
        unsigned int indiceIndex = 0;
        meshBuffers->indices = new unsigned int[meshBuffers->indicesCount];
        for (unsigned i = 0; i < aimesh->mNumFaces; ++i) {
            aiFace* face = &aimesh->mFaces[i];
            if (face->mNumIndices != 3) {
                LogError("Face has %u indices, expected 3", face->mNumIndices);
                ;
            }
            for (unsigned j = 0; j < 3U; ++j) {
                meshBuffers->indices[indiceIndex++] = face->mIndices[j];
            }
        }

        // Vertices
        meshBuffers->verticesCount = aimesh->mNumVertices;
        meshBuffers->vertices = new float3[meshBuffers->verticesCount];
        for (unsigned int i = 0; i < meshBuffers->verticesCount; ++i) {
            meshBuffers->vertices[i][0] = aimesh->mVertices[i].x;
            meshBuffers->vertices[i][1] = aimesh->mVertices[i].y;
            meshBuffers->vertices[i][2] = aimesh->mVertices[i].z;
        }

        // TexCoord
        if (aimesh->HasTextureCoords(0)) {
            meshBuffers->texcoords = new float2[meshBuffers->verticesCount];
            for (unsigned int i = 0; i < meshBuffers->verticesCount; ++i) {
                meshBuffers->texcoords[i][0] = aimesh->mTextureCoords[0][i].x;
                meshBuffers->texcoords[i][1] = 1.0f - aimesh->mTextureCoords[0][i].y;
            }
        }

        // Normals
        if (aimesh->HasNormals()) {
            meshBuffers->normals = new float3[meshBuffers->verticesCount];
            for (unsigned int i = 0; i < meshBuffers->verticesCount; ++i) {
                meshBuffers->normals[i][0] = aimesh->mNormals[i].x;
                meshBuffers->normals[i][1] = aimesh->mNormals[i].y;
                meshBuffers->normals[i][2] = aimesh->mNormals[i].z;
            }
        }

        // Material
        meshBuffers->material = aimesh->mMaterialIndex;

        instance->mTempMeshBuffers.push_back(meshBuffers);
    }

    LogInfo("Scene loaded: %u polygons", polygons);

    if (GLuint err = glGetError(); err != GL_NO_ERROR) {
        LogError("Load end OpenGL error: %u", err);
    }

    instance->mIsSceneLoaded = true;

    return (void*)(0);
}

void Model::LoadMeshData()
{
    for (auto meshBuffers : mTempMeshBuffers) {
        auto mesh = std::make_shared<Mesh>();
        if (meshBuffers->indicesCount != 0 && meshBuffers->indices != nullptr) {
            mesh->LoadIndices(meshBuffers->indices, meshBuffers->indicesCount);
            delete[] meshBuffers->indices;
        }
        if (meshBuffers->verticesCount != 0 && meshBuffers->vertices != nullptr) {
            mesh->LoadVertices(meshBuffers->vertices, meshBuffers->verticesCount);
            delete[] meshBuffers->vertices;
        }
        if (meshBuffers->verticesCount != 0 && meshBuffers->texcoords != nullptr) {
            mesh->LoadTexCoord(meshBuffers->texcoords, meshBuffers->verticesCount);
            delete[] meshBuffers->texcoords;
        }
        if (meshBuffers->verticesCount != 0 && meshBuffers->normals != nullptr) {
            mesh->LoadNormals(meshBuffers->normals, meshBuffers->verticesCount);
            delete[] meshBuffers->normals;
        }
        mesh->SetMaterial(mMaterials[meshBuffers->material]);
        mMeshes.push_back(mesh);
    }
    mTempMeshBuffers.clear();
}

void Model::Draw(ProgramModel* program)
{
    if (!mIsInitialized) {
        if (mIsSceneLoaded) {
            LoadMeshData();
            mIsInitialized = true;
        }
    } else {
        for (auto mesh : mMeshes) {
            if (auto mat = mesh->GetMaterial(); mat != nullptr) {
                program->SetAmbient(glm::value_ptr(mat->GetAmbient()));
                program->SetDiffuse(glm::value_ptr(mat->GetDiffuse()));
                program->SetSpecular(glm::value_ptr(mat->GetSpecular()));
            }
            mesh->Render(RenderPrimitive::Triangle);
        }
    }
}
