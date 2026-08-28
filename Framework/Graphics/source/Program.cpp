// Copyright 2017 NXP

#include "Program.hpp"
#include "Log.hpp"

Program::Program(const char* v_shader, const char* p_shader, const char* uTexture)
{
    mVertexShader = glCreateShader(GL_VERTEX_SHADER);
    mPixelShader = glCreateShader(GL_FRAGMENT_SHADER);

    const char* vertexShaderCode[] = {
        shaders::GlslVersion,
        (const char*)v_shader,
    };
    GLsizei vertexShaderCodeSize = sizeof(vertexShaderCode) / sizeof(char*);

    if (CompileShader(mVertexShader, vertexShaderCodeSize, vertexShaderCode) == -1) {
        LogError("VS compile failed");
        return;
    }

    const char* pixelShaderCode[] = {
        shaders::GlslVersion,
        uTexture,
        shaders::PrecisionMedium,
        (const char*)p_shader,
    };
    GLsizei pixelShaderCodeSize = sizeof(pixelShaderCode) / sizeof(char*);

    if (CompileShader(mPixelShader, pixelShaderCodeSize, pixelShaderCode) == -1) {
        LogError("PS compile failed");
        return;
    }

    mHandle = glCreateProgram();
    if (mHandle == 0U) {
        LogError("Error creating shader program object");
        return;
    }

    glAttachShader(mHandle, mVertexShader);
    glAttachShader(mHandle, mPixelShader);

    glLinkProgram(mHandle);
    // Check if linking succeeded.
    GLint linked = (GLint)0;
    glGetProgramiv(mHandle, GL_LINK_STATUS, &linked);
    if (linked == (GLint)0) {
        LogError("Program link failed");
        // Retrieve error buffer size.
        GLint errorBufSize, errorLength;
        glGetShaderiv(mHandle, GL_INFO_LOG_LENGTH, &errorBufSize);
        if (errorBufSize != (GLint)0) {
            char* infoLog = (char*)malloc((uint)errorBufSize * sizeof(char) + 1U);
            if (infoLog != NULL) {
                // Retrieve error.
                glGetProgramInfoLog(mHandle, errorBufSize, &errorLength, infoLog);
                LogInfo("%s", infoLog);
                free(infoLog);
            }
        }
    }
}

Program::~Program()
{
    if (mHandle != 0U) {
        glUseProgram(0);
        glDeleteShader(mVertexShader);
        glDeleteShader(mPixelShader);
        glDeleteProgram(mHandle);
        mHandle = 0;
    }
}

void Program::Use(void)
{
    glUseProgram(mHandle);
}

int Program::CompileShader(GLuint id, GLsizei sourceSize, const char** source)
{
    glShaderSource(id, sourceSize, source, NULL);
    glCompileShader(id);

    GLint compiled = (GLint)0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &compiled);
    if (compiled == (GLint)0) {
        // Retrieve error buffer size.
        GLint errorBufSize, errorLength;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &errorBufSize);

        char* infoLog = (char*)malloc((uint)errorBufSize * sizeof(char) + 1U);
        if (infoLog != NULL) {
            // Retrieve error.
            glGetShaderInfoLog(id, errorBufSize, &errorLength, infoLog);
            LogError("%s", infoLog);
            free(infoLog);
        }

        for (int i = 0; i < sourceSize; ++i) {
            LogInfo("%s", source[i]);
        }

        return (-1);
    }
    return 0;
}
