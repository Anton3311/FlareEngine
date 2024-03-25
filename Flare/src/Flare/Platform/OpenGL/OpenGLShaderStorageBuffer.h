#pragma once

#include "Flare/Renderer/ShaderStorageBuffer.h"

namespace Flare
{
    class OpenGLShaderStorageBuffer : public ShaderStorageBuffer
    {
    public:
        OpenGLShaderStorageBuffer(uint32_t binding);
        ~OpenGLShaderStorageBuffer();

        size_t GetSize() const override;
        void SetData(const MemorySpan& data) override;
    private:
        uint32_t m_Id = 0;
        uint32_t m_Binding = 0;
        size_t m_Size = 0;
    };
}
