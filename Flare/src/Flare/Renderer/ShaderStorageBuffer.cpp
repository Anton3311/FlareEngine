#include "ShaderStorageBuffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/OpenGL/OpenGLShaderStorageBuffer.h"

namespace Flare
{
	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(uint32_t binding)
	{
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLShaderStorageBuffer>(binding);
        }

        return nullptr;
	}
}
