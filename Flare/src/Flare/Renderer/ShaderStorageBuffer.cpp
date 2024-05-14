#include "ShaderStorageBuffer.h"

#include "Flare/Renderer/RendererAPI.h"
#include "Flare/Platform/Vulkan/VulkanShaderStorageBuffer.h"

namespace Flare
{
	Ref<ShaderStorageBuffer> ShaderStorageBuffer::Create(size_t size, uint32_t binding)
	{
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::Vulkan:
            return CreateRef<VulkanShaderStorageBuffer>(size);
        }

        return nullptr;
	}
}
