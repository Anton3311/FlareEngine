#include "ShaderStorageBuffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanShaderStorageBuffer.h"

namespace Flare
{
	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(size_t size)
	{
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::Vulkan:
            return CreateRef<VulkanShaderStorageBuffer>(size);
        }

        return nullptr;
	}
}
