#pragma once

#include "Flare/Renderer/ShaderStorageBuffer.h"

namespace Flare
{
    class OpenGLShaderStorageBuffer : public ShaderStorageBuffer
    {
    public:
        OpenGLShaderStorageBuffer(uint32_t binding);
        ~OpenGLShaderStorageBuffer();

        void SetData(const MemorySpan& data) override;
    private:
        uint32_t m_Id;
        uint32_t m_Binding;
    };
}
