/*
 * Copyright 2017, 2022 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <string>

#include <GLES3/gl3.h>
#include <glm/glm.hpp>

class Material
{
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;

    std::string texPath;

public:
    Material(glm::vec3 amb, glm::vec3 diff, glm::vec3 spec, float shin, std::string path = "")
        : ambient(amb)
        , diffuse(diff)
        , specular(spec)
        , shininess(shin)
        , texPath(path) { };
    virtual ~Material(void) = default;

    glm::vec3 GetAmbient(void) { return ambient; }
    glm::vec3 GetDiffuse(void) { return diffuse; }
    glm::vec3 GetSpecular(void) { return specular; }
};
