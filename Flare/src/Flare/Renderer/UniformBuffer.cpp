#include "UniformBuffer.h"

#include "Flare/Renderer/RendererAPI.h"

#include "Flare/Platform/Vulkan/VulkanUniformBuffer.h"

namespace Flare
{
    Ref<UniformBuffer> Flare::UniformBuffer::Create(size_t size, uint32_t binding)
    {
        switch (RendererAPI::GetAPI())
        {
        case RendererAPI::API::Vulkan:
            return CreateRef<VulkanUniformBuffer>(size);
        }

        return nullptr;
    }
}
