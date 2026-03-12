#include "ge2/Rendering/Shader.h"
#include "ge2/Core/Log.h"

#include <glad/glad.h>

namespace ge2
{
    static uint32_t CompileStage(uint32_t type, const std::string& src)
    {
        uint32_t id = glCreateShader(type);
        const char* csrc = src.c_str();
        glShaderSource(id, 1, &csrc, nullptr);
        glCompileShader(id);

        int success = 0;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            int length = 0;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
            std::string log(length, '\0');
            glGetShaderInfoLog(id, length, &length, log.data());
            Log::Error("Shader compilation failed: " + log);
            glDeleteShader(id);
            return 0;
        }
        return id;
    }

    uint32_t Shader::CompileProgram(const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        uint32_t vs = CompileStage(GL_VERTEX_SHADER, vertexSrc);
        uint32_t fs = CompileStage(GL_FRAGMENT_SHADER, fragmentSrc);
        if (!vs || !fs)
            return 0;

        uint32_t program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        int success = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success)
        {
            int length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(length, '\0');
            glGetProgramInfoLog(program, length, &length, log.data());
            Log::Error("Shader link failed: " + log);
            glDeleteProgram(program);
            program = 0;
        }

        glDetachShader(program, vs);
        glDetachShader(program, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        return program;
    }
}

