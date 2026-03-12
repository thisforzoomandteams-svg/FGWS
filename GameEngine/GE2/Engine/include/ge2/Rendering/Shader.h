#pragma once

#include <string>
#include <stdint.h>

namespace ge2
{
    class Shader
    {
    public:
        static uint32_t CompileProgram(const std::string& vertexSrc, const std::string& fragmentSrc);
    };
}

