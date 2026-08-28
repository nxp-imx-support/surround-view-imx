/*
 * Copyright 2017, 2022, 2026 NXP
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

namespace shaders {
// Vertices shaders
//------------------
// Vertices shader with view and projection parameters
constexpr const char s_v_shader_glm[] =
    "layout(location = 0) in vec4 vPosition; \n"
    "layout(location = 1) in vec2 vTexCoord; \n"
    "out vec2 TexCoord; \n"
    "uniform mat4 uTransform; \n"
    "void main() \n"
    "{ \n"
    "    gl_Position = uTransform * vec4(vPosition.xyz, 1); \n"
    "    TexCoord = vTexCoord; \n"
    "} \n";

// Vertices shader without view and projection parameters for exposure correction
constexpr const char s_v_shader[] =
    "layout(location = 0) in vec4 vPosition; \n"
    "layout(location = 1) in vec2 vTexCoord; \n"
    "out vec2 TexCoord; \n"
    "void main() \n"
    "{ \n"
    "    gl_Position = vPosition; \n"
    "    TexCoord = vTexCoord; \n"
    "} \n";

constexpr const char s_v_shader_model[] =
    "layout(location = 0) in vec3 position; \n"
    "layout(location = 2) in vec3 normal; \n"
    "\n"
    "uniform mat4 uTransform, uModelView; \n"
    "uniform mat3 uNormalMat; \n"
    "\n"
    "// light \n"
    "const vec3 lightPosition = vec3( 0.0, 0.0, 20.0 ); \n"
    "\n"
    "// material \n"
    "\n"
    "out vec3 eyePosition, eyeNormal, eyeLight; \n"
    "\n"
    "void main() \n"
    "{ \n"
    "    vec4 normPosition = uTransform*vec4(position,1); \n"
    "    gl_Position = normPosition;     \n"
    "\n"
    "    eyePosition = (uModelView*vec4(position,1)).xyz; \n"
    "    eyeLight = (uModelView*vec4(lightPosition,1)).xyz; \n"
    "    eyeNormal = normalize(uNormalMat*normal); \n"
    "} \n";

constexpr const char s_v_shader_line[] =
    "layout(location = 0) in vec4 vPosition; \n"
    "void main() \n"
    "{ \n"
    "    gl_Position = vPosition; \n"
    "    gl_PointSize = 3.0f;\n"
    "} \n";

// Fragment shaders
//------------------
// Fragment shader with color only
// To render wireframe meshes
constexpr const char FragColor[] =
    "layout(location = 0) out vec4 fragColor; \n"
    "void main() {"
    "    fragColor = vec4(1.0, 1.0, 0.0, 1.0);"
    "}";

// Fragment shader texture and gain
// To render non-overlap regions
constexpr const char FragTexture[] =
    "in vec2 TexCoord; \n"
    "out vec4 fragColor; \n"
    "uniform vec4 uGain; \n"
    "void main() \n"
    "{\n"
    "    fragColor = texture(uTexture, TexCoord) * uGain; \n"
    "}\n";

// Fragment shader with texture, mask and gain
// To render overlap regions
constexpr const char FragTextureMask[] =
    "in vec2 TexCoord; \n"
    "out vec4 fragColor; \n"
    "uniform sampler2D uMask; \n"
    "uniform vec4 uGain; \n"
    "void main() \n"
    "{\n"
    "    vec4 color = texture(uTexture, TexCoord); \n"
    "    fragColor = vec4(color.rgb, texture(uMask, TexCoord).r * color.a) * uGain; \n"
    "}\n";

// Fragment shader with Phong lighting
// To render car 3D model
constexpr const char FragPhong[] =
    "uniform vec3 uAmbient; \n"
    "uniform vec3 uDiffuse; \n"
    "uniform vec3 uSpecular; \n"
    "\n"
    "in vec3 eyePosition, eyeNormal, eyeLight; \n"
    "\n"
    "out vec4 fragColor; \n"
    "\n"
    "vec3 lightColor = vec3(1.0); \n"
    "void main() \n"
    "{ \n"
    "    vec3 N = normalize(eyeNormal); \n"
    "    vec3 L = normalize(eyeLight-eyePosition); \n"
    "    vec3 finalColor = uAmbient; \n"
    "\n"
    "    //Blinn-Phong model \n"
    "    float lambertTerm = dot(L, N); \n"
    "    if(lambertTerm >= 0.0) \n"
    "    { \n"
    "\n"
    "        //Phong model \n"
    "        vec3 V = normalize(-eyePosition); \n"
    "        vec3 R = reflect(-L, N); \n"
    "        float spec = pow(max(0.0, dot(R, V)), 128.0); \n"
    "\n"
    "        finalColor += lightColor * (uSpecular * spec + uDiffuse * lambertTerm); \n"
    "    } \n"
    "\n"
    "    fragColor = vec4(finalColor, 1.0); \n"
    "} \n";

// Fragment shader with color only
// To render wireframe meshes
constexpr const char FragBox[] =
    "in vec2 TexCoord; \n"
    "out vec4 fragColor; \n"
    "uniform vec4 uGain; \n"
    "void main() {"
    "    if (TexCoord.x >= uGain.y && (1.0-TexCoord.y) >= uGain.x && TexCoord.x <= uGain.w && (1.0-TexCoord.y) <= uGain.z) {"
    "        fragColor = vec4(0.3);"
    "    }"
    "    else {"
    "        fragColor = vec4(0.0);"
    "    }"
    "}";
}
